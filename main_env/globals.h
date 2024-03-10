#include "env.h"
#include "DFRobot_BME680_I2C.h"
#include "Wire.h"
#include "ArduinoJson.h"
#include "MQTT.h"
#include "ESP8266WiFi.h"

#define TYPE "bme680_outside" // "bme680_inside"

DFRobot_BME680_I2C bme(0x76);

const float altitude = 217.0;

float sea_level;

JsonDocument data;

const char ssid[] = SSID_ENV;
const char pass[] = PASSWORD_ENV;

const char mqtt_host[] = MQTT_HOST_ENV;
const int mqtt_port = MQTT_PORT_ENV;

MQTTClient mqtt_client;
WiFiClient wifi_client;

unsigned long lastMillis = 0;
