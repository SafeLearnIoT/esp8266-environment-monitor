#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "bsec.h"
#include "communication.h"
#include "env.h"
#include "data.h"

inline bool upload_init = false;
inline String header = "timestamp,temperature,pressure,humidity,gas_resistance,iaq\n";
inline Bsec iaqSensor;
inline unsigned long lastDataSaveMillis = 0;
inline unsigned long lastDataUploadMillis = 0;

void checkIaqSensorStatus(void);

void callback(String &topic, String &payload);

String get_todays_file_path();

String get_yesterdays_file_path();

void read_data();

inline auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp8266/inside/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback); // esp8266/outside
#endif