#include "utils.h"

// Helper function definitions
void checkIaqSensorStatus(void)
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

void callback(String &topic, String &payload)
{
    Serial.print("Topic: ");
    Serial.print(topic);
    Serial.print(", Payload: ");
    Serial.println(payload);
}

String get_todays_file_path()
{
    return "/" + comm->get_todays_date_string() + ".csv";
}

String get_yesterdays_file_path()
{

    return "/" + comm->get_yesterdays_date_string() + ".csv";
}

void read_data()
{
    String output;
    if (iaqSensor.run())
    { // If new data is available
        output = String(comm->get_rawtime());
        output += "," + String(iaqSensor.temperature);
        output += "," + String(iaqSensor.pressure);
        output += "," + String(iaqSensor.humidity);
        output += "," + String(iaqSensor.gasResistance);
        if (iaqSensor.iaqAccuracy != 0)
        {
            output += "," + String(iaqSensor.iaq);
        }
        else
        {
            output += ",";
        }
        output += "\n";
        digitalWrite(LED_BUILTIN, HIGH);
        if (millis() - lastDataSaveMillis > 900000) // 60000 - minute
        {
            lastDataSaveMillis = millis();

            comm->resume_communication();

            if (!LittleFS.exists(get_todays_file_path()))
            {
                writeFile(LittleFS, get_todays_file_path().c_str(), header.c_str());
            }
            appendFile(LittleFS, get_todays_file_path().c_str(), output.c_str());

            delay(5000);

            comm->pause_communication();
        }
    }
    else
    {
        checkIaqSensorStatus();
    }
}