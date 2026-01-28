/**
 *               FreeRTOS Tasks
 *               CCSIPRO™
 **/

//==================================================================
// serialTask - čítanie Serial portu a odosielanie do queue
//==================================================================
void serialTask(void* parameter) {
  static String buffer;
  const size_t MAX_SERIAL_BUFFER = 4096; // Max 4KB pre serial buffer
  
  while (true) {
    while (Serial.available()) {
      char c = Serial.read();
      
      if (c == '\n' || c == '\r') {
        buffer.trim();
        
        if (buffer.startsWith("{") && buffer.endsWith("}")) {
          QueueItem item;
          buffer.toCharArray(item.jsonCommand, QUEUE_JSON_SIZE);
          strcpy(item.source, "serial");
          xQueueSend(commandQueue, &item, 0);
        } else if (buffer.length() > 0) {
          Serial.print(F("Serial non-JSON: "));
          Serial.println(buffer);
        }
        buffer = "";
      } else {
        // Ochrana proti pretečeniu buffera
        if (buffer.length() < MAX_SERIAL_BUFFER) {
          buffer += c;
        } else {
          Serial.println(F("ERROR: Serial buffer overflow, resetting"));
          buffer = "";
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // 50ms delay = 20 kontrol/sec
  }
}

//==================================================================
// mqttTask - MQTT handling (pripájanie, reconnect, loop)
//==================================================================
void mqttTask(void* parameter) {
  while (true) {
    if (is_mqtt_allowed && stav.lan == 1) {
      if (!mqtt.connected()) {
        uint32_t now = millis();
        
        // Počkáme 10s od posledného pokusu
        if ((now - lastReconnectAttempt) >= 10000) {
          lastReconnectAttempt = now;
          bool status = mqttConnect();
          
          if (!status) {
            last_connection_attempt++;
            Serial.println("=== MQTT NOT CONNECTED ===");
            if (last_connection_attempt > 3) {
              Serial.println("Restarting...");
              ESP.restart();
            }
          } else {
            last_connection_attempt = 0;
          }
        }
      } else {
        // Sme pripojení - vynulujeme počitadlo
        last_connection_attempt = 0;
        mqtt.loop();
      }
    } else {
      // MQTT nie je povolené alebo ethernet nie je pripojený
      mqtt.disconnect();
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay
  }
}

//==================================================================
// apiTask - HTTP API server handling
//==================================================================
void apiTask(void* parameter) {
  while (true) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(50)); // 50ms delay = 20 kontrol/sec
  }
}

//==================================================================
// relayTask - spracovanie príkazov z queue a kontrola timerov relé
//==================================================================
void relayTask(void* parameter) {
  QueueItem item;
  
  while (true) {
    // 1. Spracuj príkazy z fronty
    if (xQueueReceive(commandQueue, &item, 0) == pdTRUE) {
      // Debug: vypíš obsah a dĺžku JSON správy (len ak je verbose)
      if (verbose) {
        Serial.print("Processing JSON from ");
        Serial.print(item.source);
        Serial.print(" (len=");
        Serial.print(strlen(item.jsonCommand));
        Serial.print("): [");
        Serial.print(item.jsonCommand);
        Serial.println("]");
      }
      
      DynamicJsonDocument doc(8192);
      DeserializationError err = deserializeJson(doc, item.jsonCommand);
      
      if (!err) {
        String result = handleJson(doc, String(item.source));
        
        // Odošli odpoveď podľa zdroja
        if (strcmp(item.source, "serial") == 0) {
          // Pre Serial - JSON output
          DynamicJsonDocument outDoc(8192);
          DeserializationError innerErr = deserializeJson(outDoc["result"], result);
          
          if (innerErr) {
            outDoc["result"] = result;
          }
          
          String response;
          serializeJson(outDoc, response);
          Serial.println(response);
          
        } else if (strcmp(item.source, "mqtt") == 0) {
          // Pre MQTT - priama publikácia
          int resultLength = result.length();
          if (resultLength < SAFE_LENGTH) {
            mqtt.publish(nazov_odosielacieho_kanala.c_str(), result.c_str());
          } else {
            if (mqtt.beginPublish(nazov_odosielacieho_kanala.c_str(), resultLength, false)) {
              mqtt.write((const uint8_t*)result.c_str(), resultLength);
              mqtt.endPublish();
            }
          }
        }
        // Pre "api" source sa odpoveď odosiela priamo v api.ino handleri
      } else {
        Serial.print(F("JSON deserialization error: "));
        Serial.println(err.f_str());
      }
    }
    
    // 2. Kontrola timerov relé
    currentMillis = millis();
    long currentMillisLong = (long)currentMillis;
    
    for (itr = koniec.begin(); itr != koniec.end(); itr++) {
      if ((itr->second > 0) && (itr->second < currentMillisLong)) {
        vypni(itr->first);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(50)); // 50ms = 20 kontrol/sec
  }
}

//==================================================================
// rgbLedTask - RGB LED blikanie a stavová indikácia
//==================================================================
void rgbLedTask(void* parameter) {
  while (true) {
    processRgbBlink();
    vTaskDelay(pdMS_TO_TICKS(50)); // 50ms = 20Hz refresh
  }
}