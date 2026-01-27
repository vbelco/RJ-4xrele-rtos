void mqttCallback(char* topic, uint8_t* payload, unsigned int len) {
  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  // Konverzia payload na String
  String messageTemp;
  for (unsigned int i = 0; i < len; i++) {
    messageTemp += (char)payload[i];
  }
  
  // Odoslať do command queue pre spracovanie v relayTask
  QueueItem item;
  item.jsonCommand = messageTemp;
  item.source = "mqtt";
  
  if (xQueueSend(commandQueue, &item, 0) != pdTRUE) {
    Serial.println("Failed to send MQTT command to queue!");
    mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Queue full - command rejected");
  }
}