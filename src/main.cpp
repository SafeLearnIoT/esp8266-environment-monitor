#define ESP8266

#include "env.h"
#include "communication.h"
#include "bme_data_collector.h"

const char ssid[] = SSID_ENV;
const char pass[] = PASSWORD_ENV;

const char mqtt_host[] = MQTT_HOST_ENV;
const int mqtt_port = MQTT_PORT_ENV;

unsigned long lastMillis = 0;

void callback(String &topic, String &payload)
{
  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print(", Payload: ");
  Serial.println(payload);
}

Communication comm(SSID_ENV, PASSWORD_ENV, "/esp8266/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(1000);
  setup_bme();
  delay(2000);
  comm.setup();
}

void loop()
{
  comm.handle_mqtt_loop();
  delay(2000);
  comm.publish(TYPE, read_data());
}