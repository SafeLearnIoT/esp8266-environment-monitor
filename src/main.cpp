#ifndef ESP8266
#define ESP8266
#endif

#include <Arduino.h>
#include "bsec.h"
#include "communication.h"
#include "ml.h"
#include "env.h"

ML *temperature_ml;
ML *humidity_ml;
ML *pressure_ml;
ML *iaq_ml;

Bsec iaqSensor;
unsigned long lastDataSaveMillis = 0;

void callback(String &topic, String &payload)
{
    Serial.print("Topic: ");
    Serial.print(topic);
    Serial.print(", Payload: ");
    Serial.println(payload);
}

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp8266/outside", MQTT_HOST_ENV, MQTT_PORT_ENV, callback); // esp8266/outside

// Helper function definitions
void check_iaq_sensor_status()
{
    String output;
    if (iaqSensor.bsecStatus != BSEC_OK)
    {
        if (iaqSensor.bsecStatus < BSEC_OK)
        {
            output = "BSEC error code : " + String(iaqSensor.bsecStatus);
            Serial.println(output);
        }
        else
        {
            output = "BSEC warning code : " + String(iaqSensor.bsecStatus);
            Serial.println(output);
        }
    }

    if (iaqSensor.bme68xStatus != BME68X_OK)
    {
        if (iaqSensor.bme68xStatus < BME68X_OK)
        {
            output = "BME68X error code : " + String(iaqSensor.bme68xStatus);
            Serial.println(output);
        }
        else
        {
            output = "BME68X warning code : " + String(iaqSensor.bme68xStatus);
            Serial.println(output);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    delay(1000);

    iaqSensor.begin(BME68X_I2C_ADDR_LOW, Wire);
    String output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
    Serial.println(output);
    check_iaq_sensor_status();

    bsec_virtual_sensor_t sensorList[13] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_GAS_PERCENTAGE};

    iaqSensor.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
    check_iaq_sensor_status();

    delay(2000);

    comm->setup();

    delay(5000);

    comm->pause_communication();

    switch (comm->get_ml_algo())
    {
    case MLAlgo::LinReg:
        temperature_ml = new Regression::Linear(SensorType::Temperature);
        humidity_ml = new Regression::Linear(SensorType::Humidity);
        pressure_ml = new Regression::Linear(SensorType::Pressure);
        iaq_ml = new Regression::Linear(SensorType::IAQ);
        break;
    case MLAlgo::LogReg:
        temperature_ml = new Regression::Logistic(SensorType::Temperature);
        humidity_ml = new Regression::Logistic(SensorType::Humidity);
        pressure_ml = new Regression::Logistic(SensorType::Pressure);
        iaq_ml = new Regression::Logistic(SensorType::IAQ);
        break;
    case MLAlgo::rTPNN:
        temperature_ml = new RTPNN::SDP(SensorType::Temperature);
        humidity_ml = new RTPNN::SDP(SensorType::Humidity);
        pressure_ml = new RTPNN::SDP(SensorType::Pressure);
        iaq_ml = new RTPNN::SDP(SensorType::IAQ);
        break;
    case MLAlgo::None:
        break;
    }
}

void loop()
{
    if (iaqSensor.run())
    {                                              // If new data is available
        if (millis() - lastDataSaveMillis > 10000) // 60000 - minute
        {
            lastDataSaveMillis = millis();

            auto raw_time = comm->get_rawtime();
            auto time_struct = localtime(&raw_time);

            JsonDocument sensor_data;
            sensor_data["time"] = raw_time;
            sensor_data["device"] = comm->get_client_id();
            JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
            detail_sensor_data["temperature"] = iaqSensor.temperature;
            detail_sensor_data["pressure"] = iaqSensor.pressure;
            detail_sensor_data["humidity"] = iaqSensor.humidity;
            if (iaqSensor.iaqAccuracy != 0)
            {
                detail_sensor_data["iaq"] = iaqSensor.iaq;
            }

            JsonDocument ml_data;
            if (comm->get_ml_algo() != MLAlgo::None)
            {
                ml_data["time"] = raw_time;
                ml_data["device"] = comm->get_client_id();
                ml_data["ml_algo"] = "reglin";
                JsonObject detail_reglin_data = ml_data["data"].to<JsonObject>();

                JsonObject temperature_data = detail_reglin_data["temperature"].to<JsonObject>();
                temperature_data = temperature_ml->perform(*time_struct, iaqSensor.temperature);

                JsonObject pressure_data = detail_reglin_data["pressure"].to<JsonObject>();
                pressure_data = pressure_ml->perform(*time_struct, iaqSensor.pressure);

                JsonObject humidity_data = detail_reglin_data["humidity"].to<JsonObject>();
                humidity_data = humidity_ml->perform(*time_struct, iaqSensor.humidity);

                if (iaqSensor.iaqAccuracy != 0)
                {
                    JsonObject iaq_data = detail_reglin_data["iaq"].to<JsonObject>();
                    iaq_data = iaq_ml->perform(*time_struct, iaqSensor.iaq);
                }
            }
            comm->send_data(sensor_data, ml_data);
        }
    }
    else
    {
        check_iaq_sensor_status();
    }
}