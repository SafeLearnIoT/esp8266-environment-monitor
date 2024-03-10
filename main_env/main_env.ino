#include "globals.h"


void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(2000);
  WiFi.begin(ssid, pass);
  delay(2000);
  mqtt_setup();
  delay(2000);
  mqtt_connect();
  delay(2000);
  setup_bme();
}


void loop() {
  mqtt_client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!mqtt_client.connected()) {
    mqtt_connect();
  }

  read_data();
  delay(2000);
}
