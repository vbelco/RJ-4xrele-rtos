boolean mqttConnect()
{
  Serial.println("Pripajam k brokeru, resetujem WDT...");
  esp_task_wdt_reset();
    
  Serial.print("Connecting to ");
  Serial.print(broker);

  // authenticate MQTT
  //  boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
  boolean status = mqtt.connect(nazov_clienta.c_str(), mqtt_username.c_str(), mqtt_password.c_str(), nazov_lwt.c_str(), 2, false, nazov_clienta.c_str());

  if (status == false)
  {
    Serial.println(" fail");
    stav.mqtt = 0;       // MQTT DOWN
    updateLedFromStav();
    return false;
  }
  Serial.println(" success");
  mqtt.publish(nazov_online.c_str(), nazov_clienta.c_str()); // po pripojeni posleme spravu do kanala
  mqtt.subscribe(nazov_prijimacieho_kanala.c_str());
  stav.mqtt = 1;       // MQTT OK
  updateLedFromStav();
  return mqtt.connected();
}

//  --------------------------------------
//   OBSLUHA PRICHADZAJUCICH SPRAV MQTT - FreeRTOS version
//  --------------------------------------
void mqttCallback(char* topic, uint8_t* payload, unsigned int len) {
  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  // Konverzia payload do QueueItem
  QueueItem item;
  
  // Skopíruj payload do item.jsonCommand (s ochranou proti pretečeniu)
  unsigned int copyLen = (len < QUEUE_JSON_SIZE - 1) ? len : (QUEUE_JSON_SIZE - 1);
  memcpy(item.jsonCommand, payload, copyLen);
  item.jsonCommand[copyLen] = '\0'; // Null terminátor
  
  strcpy(item.source, "mqtt");
  
  if (xQueueSend(commandQueue, &item, 0) != pdTRUE) {
    Serial.println("Failed to send MQTT command to queue!");
    mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Queue full - command rejected");
  }
}