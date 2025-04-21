#ifndef ESP8266
#define ESP8266
#endif

#include <Arduino.h>
#include "bsec.h"
#include "communication.h"
#include "jsonToArray.h"
#include "ml.h"
#include "env.h"

void callback(String &topic, String &payload);
auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp8266/inside", MQTT_HOST_ENV, MQTT_PORT_ENV, callback); // esp8266/outside

auto temperature_ml = ML(Temperature, "temperature");
auto humidity_ml = ML(Humidity, "humidity");
auto pressure_ml = ML(Pressure, "pressure");
auto iaq_ml = ML(IAQ, "iaq");

Bsec iaqSensor;
unsigned long lastDataSaveMillis = 0;

bool get_params = false;
bool set_params = false;
String payload_content;

void callback(String &topic, String &payload)
{
    if (topic == "configuration" && !comm->is_system_configured())
    {
        comm->initConfig(payload);
        return;
    }
    Serial.println("[SUB][" + topic + "] " + payload);
    if (payload == "get_params")
    {
        if (!get_params)
        {
            get_params = true;
        }
        return;
    }
    if (payload.startsWith("set_params;"))
    {
        if (!set_params)
        {
            payload_content = payload;
            set_params = true;
        }
        return;
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

    delay(5000);
}

void loop()
{
    if (comm->is_system_configured())
    {
        if (get_params)
        {
            comm->hold_connection();

            JsonDocument doc;
            auto raw_time = comm->get_rawtime();

            doc["time_sent"] = raw_time;
            doc["device"] = comm->get_client_id();

            JsonObject data = doc["data"].to<JsonObject>();
            

            data["type"] = "params";
            temperature_ml.get_params(data);
            humidity_ml.get_params(data);
            pressure_ml.get_params(data);
            iaq_ml.get_params(data);
            comm->publish("cmd_mcu", "params;" + doc.as<String>());
            get_params = false;
        }
        if (set_params)
        {
            JsonDocument doc;
            if (payload_content.substring(11) == "ok")
            {
                comm->publish("cmd_gateway", "", true);
                comm->release_connection();
                payload_content.clear();
                set_params = false;
                return;
            }
            deserializeJson(doc, payload_content.substring(11));

            auto temperature_weights = Converter<std::array<double, 8>>::fromJson(doc["temperature"]);
            Serial.print("[Temperature]");
            temperature_ml.set_params(temperature_weights);

            auto humidity_weights = Converter<std::array<double, 8>>::fromJson(doc["humidity"]);
            Serial.print("[Humidity]");
            humidity_ml.set_params(humidity_weights);

            auto pressure_weights = Converter<std::array<double, 8>>::fromJson(doc["pressure"]);
            Serial.print("[Pressure]");
            pressure_ml.set_params(pressure_weights);

            auto iaq_weights = Converter<std::array<double, 8>>::fromJson(doc["iaq"]);
            Serial.print("[IAQ]");
            iaq_ml.set_params(iaq_weights);

            // Clear message
            comm->publish("cmd_gateway", "", true);

            comm->release_connection();
            payload_content.clear();
            set_params = false;
        }

        comm->handle_mqtt_loop();

        if (iaqSensor.run())
        {                                               // If new data is available
            if (millis() - lastDataSaveMillis > comm->m_configuration->getActionIntervalMillis()) // 60000 - minute; 900000 - 15 minutes
            {
                comm->resume_communication();
                lastDataSaveMillis = millis();
                auto raw_time = comm->get_rawtime();

                if (comm->m_configuration->getSendSensorData())
                {
                    JsonDocument sensor_data;
                    sensor_data["time_sent"] = raw_time;
                    sensor_data["device"] = comm->get_client_id();

                    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
                    detail_sensor_data["test_name"] = comm->m_configuration->getTestName();
                    detail_sensor_data["temperature"] = iaqSensor.temperature;
                    detail_sensor_data["pressure"] = iaqSensor.pressure;
                    detail_sensor_data["humidity"] = iaqSensor.humidity;
                    if (iaqSensor.iaqAccuracy != 0)
                    {
                        detail_sensor_data["iaq"] = iaqSensor.iaq;
                    }

                    comm->send_data(sensor_data);
                }

                if (comm->m_configuration->getRunMachineLearning())
                {
                    JsonDocument ml_data;
                    ml_data["time_sent"] = raw_time;
                    ml_data["device"] = comm->get_client_id();

                    JsonObject detail_ml_data = ml_data["data"].to<JsonObject>();
                    detail_ml_data["type"] = "prediction";
                    detail_ml_data["test_name"] = comm->m_configuration->getTestName();

                    JsonObject temperature_data = detail_ml_data["temperature"].to<JsonObject>();
                    JsonObject pressure_data = detail_ml_data["pressure"].to<JsonObject>();
                    JsonObject humidity_data = detail_ml_data["humidity"].to<JsonObject>();

                    auto training_mode = comm->m_configuration->getMachineLearningTrainingMode();
                    temperature_ml.perform(iaqSensor.temperature, temperature_data, training_mode);
                    pressure_ml.perform(iaqSensor.pressure, pressure_data, training_mode);
                    humidity_ml.perform(iaqSensor.humidity, humidity_data, training_mode);

                    if (iaqSensor.iaqAccuracy != 0)
                    {
                        JsonObject iaq_data = detail_ml_data["iaq"].to<JsonObject>();
                        iaq_ml.perform(iaqSensor.iaq, iaq_data, training_mode);
                    }
                    comm->send_ml(ml_data);
                }
            }
        }
        else
        {
            check_iaq_sensor_status();
        }
    }
    else
    {
        Serial.println("System not configured");
        comm->handle_mqtt_loop();
        delay(1000);
    }
}