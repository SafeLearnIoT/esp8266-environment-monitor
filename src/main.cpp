#ifndef ESP8266
#define ESP8266
#endif

#include <Arduino.h>
#include "bsec.h"
#include "communication.h"
#include "rtpnn.h"
#include "env.h"

inline String data_header = "timestamp,temperature,pressure,humidity,iaq\n";
inline String rtpnn_header = "temperature_trend,temperature_level,pressure_trend,pressure_level,humidity_trend,humidity_level,iaq_trend,iaq_level\n";

inline Bsec iaqSensor;
inline unsigned long lastDataSaveMillis = 0;

inline rTPNN::SDP<float> temperature_sdp(rTPNN::SDPType::Temperature);
inline rTPNN::SDP<float> humidity_sdp(rTPNN::SDPType::Humidity);
inline rTPNN::SDP<float> pressure_sdp(rTPNN::SDPType::Pressure);
inline rTPNN::SDP<float> iaq_sdp(rTPNN::SDPType::IAQ);

void callback(String &topic, String &payload)
{
    Serial.print("Topic: ");
    Serial.print(topic);
    Serial.print(", Payload: ");
    Serial.println(payload);
}

inline auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp8266/outside", MQTT_HOST_ENV, MQTT_PORT_ENV, callback); // esp8266/outside

// Helper function definitions
void checkIaqSensorStatus()
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
    checkIaqSensorStatus();

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
    checkIaqSensorStatus();

    delay(2000);

    comm->setup();

    delay(5000);

    comm->pause_communication();
}

void loop()
{
    if (iaqSensor.run())
    {                                              // If new data is available
        if (millis() - lastDataSaveMillis > 60000) // 60000 - minute
        {
            lastDataSaveMillis = millis();

            JsonDocument sensor_data;
            sensor_data["time"] = comm->get_rawtime();
            sensor_data["device"] = comm->get_client_id();
            JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
            detail_sensor_data["temperature"] = iaqSensor.temperature;
            detail_sensor_data["pressure"] = iaqSensor.pressure;
            detail_sensor_data["humidity"] = iaqSensor.humidity;
            if (iaqSensor.iaqAccuracy != 0)
            {
                detail_sensor_data["iaq"] = iaqSensor.iaq;
            }
            JsonDocument rtpnn_data;
            rtpnn_data["time"] = comm->get_rawtime();
            rtpnn_data["device"] = comm->get_client_id();
            rtpnn_data["ml_algo"] = "rtpnn";
            JsonObject detail_rtpnn_data = rtpnn_data["data"].to<JsonObject>();

            JsonObject temperature_data = detail_rtpnn_data["temperature"].to<JsonObject>();
            auto temperature_calc = temperature_sdp.execute_sdp(iaqSensor.temperature);
            temperature_data["trend"] = temperature_calc.first;
            temperature_data["level"] = temperature_calc.second;

            JsonObject pressure_data = detail_rtpnn_data["pressure"].to<JsonObject>();
            auto pressure_calc = pressure_sdp.execute_sdp(iaqSensor.pressure);
            pressure_data["trend"] = pressure_calc.first;
            pressure_data["level"] = pressure_calc.second;

            JsonObject humidity_data = detail_rtpnn_data["humidity"].to<JsonObject>();
            auto humidity_calc = humidity_sdp.execute_sdp(iaqSensor.humidity);
            humidity_data["trend"] = humidity_calc.first;
            humidity_data["level"] = humidity_calc.second;

            if (iaqSensor.iaqAccuracy != 0)
            {
                JsonObject iaq_data = detail_rtpnn_data["iaq"].to<JsonObject>();
                auto iaq_calc = iaq_sdp.execute_sdp(iaqSensor.iaq);
                iaq_data["trend"] = iaq_calc.first;
                iaq_data["level"] = iaq_calc.second;
            }
            JsonDocument reglin_data;

            comm->send_data(sensor_data, rtpnn_data, reglin_data);
        }
    }
    else
    {
        checkIaqSensorStatus();
    }
}