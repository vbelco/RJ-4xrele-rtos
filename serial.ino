// ======== Čítačka JSON zo Serialu ================================
void processSerial() {
  static String buffer;

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      buffer.trim();

      if (buffer.startsWith("{") && buffer.endsWith("}")) {
        DynamicJsonDocument doc(8192);
        DeserializationError err = deserializeJson(doc, buffer);

        if (err) {
          Serial.print(F("Serial JSON error: "));
          Serial.println(err.f_str());
        } else {
          String result = handleJson(doc);

          // Pokus o interpretáciu výsledku ako JSON
          DynamicJsonDocument outDoc(8192);
          DeserializationError innerErr = deserializeJson(outDoc["result"], result);

          // Ak to nebol platný JSON, použijeme ako string
          if (innerErr) {
            outDoc["result"] = result;
          }

          String response;
          serializeJsonPretty(outDoc, response);
          Serial.println(response);
        }
      } else if (buffer.length() > 0) {
        Serial.print(F("Serial non-JSON: "));
        Serial.println(buffer);
      }
      buffer = "";
    } else {
      buffer += c;
    }
  }
}
