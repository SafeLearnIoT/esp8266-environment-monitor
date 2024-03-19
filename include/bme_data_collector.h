#ifndef BME_DATA_COLLECTOR_H
#define BME_DATA_COLLECTOR_H

#include "Arduino.h"
#include "DFRobot_BME680_I2C.h"
#include "ArduinoJson.h"

#define TYPE "bme680_outside" // bme680_outside, bme680_inside

void setup_bme();
String read_data();

#endif