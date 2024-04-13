#ifndef ESP8266
#define ESP8266
#endif

#include "utils.h"

void setup()
{
    Serial.begin(115200);

    if (!LittleFS.begin())
    {
        Serial.println("LittleFS Mount Failed");
        return;
    }

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
    read_data();

    auto current_time = comm->get_localtime();
    if (current_time->tm_hour == 1 && !upload_init)
    {
        Serial.println("Start reading data");
        comm->resume_communication();
        delay(5000);
        comm->handle_mqtt_loop();
        upload_init = true;
        comm->publish("data", readFile(LittleFS, get_yesterdays_file_path().c_str()));
        checkAndCleanFileSystem(LittleFS);
        delay(5000);
        comm->pause_communication();
    }
    if (current_time->tm_hour == 2 && upload_init)
    {
        listDir(LittleFS, "/");
        upload_init = false;
    }

    // DEBUG:
    // if (millis() - lastDataUploadMillis > 60000) // 60000 - minute
    // {
    //     lastDataUploadMillis = millis();
    //     auto out = readFile(LittleFS, get_todays_file_path().c_str());
    //     comm->publish("data", out);
    //     listDir(LittleFS, "/");
    // }
}