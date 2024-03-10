void setup_bme(){
  uint8_t result = 1;
  
  Serial.println();
  while (result != 0) {
    result = bme.begin();
    if (result != 0) {
      Serial.print("bme ");
      Serial.print(TYPE);
      Serial.println(" begin failure");
      delay(2000);
    }
  }
  Serial.print("bme ");
  Serial.print(TYPE);
  Serial.println(" begin successful");
  bme.supportIAQ();
}

void read_data() {
  data.clear();
  static uint8_t calibrated = 0;

  if (calibrated == 0) {
    if (bme.iaqUpdate() == 0) {
      sea_level = bme.readSeaLevel(altitude);
      calibrated = 1;
    }
  }

  if (calibrated) {
    uint8_t rslt = bme.iaqUpdate();
    if (rslt == 0) {
      data["type"] = TYPE;
      data["timestamp"] = millis();
      data["temperature"] = bme.readTemperature();
      data["pressure"] = bme.readPressure();
      data["humidity"] = bme.readHumidity();
      data["gas resistance"] = bme.readGasResistance();
      data["altitude(m)"] = bme.readAltitude();
      data["calibrated altitude(m)"] = bme.readCalibratedAltitude(sea_level);
      if (bme.isIAQReady()) {
        data["IAQ"] = bme.readIAQ();
      }
      else{
        data["IAQ"] = nullptr;
        Serial.print("[IAQ not ready, please wait about ");
        Serial.print((int)(305000 - millis()) / 1000);
        Serial.println(" seconds]");
      }
      String output = "";
      String topic = "/esp8266/";
      topic += TYPE;
      serializeJsonPretty(data, output);
      mqtt_client.publish(topic.c_str(), output.c_str());
      Serial.println();
    }
  }
}