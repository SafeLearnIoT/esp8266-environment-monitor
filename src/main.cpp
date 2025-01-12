#ifndef ESP8266
#define ESP8266
#endif

#include <Arduino.h>
#include "bsec.h"
#include "communication.h"
#include "ml.h"
#include "env.h"

auto temperature_ml = ML(Temperature, "temperature");
auto humidity_ml = ML(Humidity, "humidity");
auto pressure_ml = ML(Pressure, "pressure");
auto iaq_ml = ML(IAQ, "iaq");

Bsec iaqSensor;
unsigned long lastDataSaveMillis = 0;

void callback(String &topic, String &payload);

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp8266/outside", MQTT_HOST_ENV, MQTT_PORT_ENV, callback); // esp8266/outside

void callback(String &topic, String &payload)
{
    Serial.println("[SUB][" + topic + "] " + payload);
    if(payload == "get_weights")
    {
        comm->hold_connection();
        comm->publish("cmd", "", true);
        auto weights = temperature_ml.get_weights();
        serializeJson(weights, Serial);
        Serial.println();
    }
    if(payload=="set_weights")
    {
        comm->release_connection();
    }
}


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
}

void loop()
{
    comm->handle_mqtt_loop();
    if (iaqSensor.run())
    {                                              // If new data is available
        if (millis() - lastDataSaveMillis > 10000) // 60000 - minute; 900000 - 15 minutes
        {
            comm->resume_communication();
            lastDataSaveMillis = millis();

            auto raw_time = comm->get_rawtime();
            JsonDocument sensor_data;
            sensor_data["time_sent"] = raw_time;
            sensor_data["device"] = comm->get_client_id();
            JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
            detail_sensor_data["temperature"] = iaqSensor.temperature;
            detail_sensor_data["pressure"] = iaqSensor.pressure;
            detail_sensor_data["humidity"] = iaqSensor.humidity;
            if (iaqSensor.iaqAccuracy != 0)
            {
                detail_sensor_data["iaq"] = iaqSensor.iaq;
            }

            temperature_ml.perform(iaqSensor.temperature);
            pressure_ml.perform(iaqSensor.pressure);
            humidity_ml.perform(iaqSensor.humidity);

            if (iaqSensor.iaqAccuracy != 0)
            {
                iaq_ml.perform(iaqSensor.iaq);
            }
            serializeJson(sensor_data, Serial);
            Serial.println();
  
            comm->send_data(sensor_data);
        }
    }
    else
    {
        check_iaq_sensor_status();
    }
}