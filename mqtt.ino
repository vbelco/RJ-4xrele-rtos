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
//   OBSLUHA PRICHADZAJUCICH SPRAV MQTT
//  --------------------------------------
void mqttCallback(char* topic, uint8_t* payload, unsigned int len) {
  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  DynamicJsonDocument doc(8192);  // Nastavenie veľkosti podľa odhadovanej potreby, zopoveda velkosti mqtt spravy
  DeserializationError err = deserializeJson(doc, payload);
  Serial.print("Deserialization: ");
  Serial.println(err.f_str());

  if (err) {
    mqtt.publish(nazov_odosielacieho_kanala.c_str(), "invalid JSON");
    return;
  }

  String result = "";
  result = handleJson(doc, "mqtt");

  int resultLength = result.length();
  if (result.length() < SAFE_LENGTH) {
    mqtt.publish(nazov_odosielacieho_kanala.c_str(), result.c_str() );
  } else {
    if (mqtt.beginPublish(nazov_odosielacieho_kanala.c_str(), resultLength, false)) {
        mqtt.write((const uint8_t*)result.c_str(), resultLength);
        mqtt.endPublish();
        Serial.println("OK");
    } else Serial.println("FAIL");
  }
}
