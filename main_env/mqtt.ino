void mqtt_setup(){
  mqtt_client.begin(mqtt_host, mqtt_port, wifi_client);
  mqtt_client.onMessage(callback);
}

void mqtt_connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  String client_id = "esp8266_";
  client_id += TYPE;
  while (!mqtt_client.connect(client_id.c_str())) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");
}

void callback(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}