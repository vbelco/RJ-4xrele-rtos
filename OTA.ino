// ============================
// FUNKCIA: OTA AKTUALIZÁCIA - v3 (HTTP/HTTPS)
// ============================
bool performOTAUpdate(const String &host, const String &port, const String &path, const String& request_from) {
  Serial.println("Spustam OTA update...");
  if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "[OTA] Spustam OTA update..." );

  // Detekcia HTTP vs HTTPS podľa portu
  int portNumber = port.toInt();
  bool useHTTPS = (portNumber == 443);
  
  // Vytvorenie správneho typu klienta
  WiFiClient* updateClient = nullptr;
  WiFiClientSecure* secureClient = nullptr;
  
  if (useHTTPS) {
    secureClient = new WiFiClientSecure();
    secureClient->setInsecure(); // Akceptuje self-signed certifikáty
    updateClient = secureClient;
    Serial.printf("Pripajam sa na server cez HTTPS: %s:%d\n", host.c_str(), portNumber);
  } else {
    updateClient = new WiFiClient();
    Serial.printf("Pripajam sa na server cez HTTP: %s:%d\n", host.c_str(), portNumber);
  }

  // 1) Pripojenie k serveru
  char msg[160];
  snprintf(msg, sizeof(msg),"[OTA] Pripajam sa na server (%s): %s:%u", useHTTPS ? "HTTPS" : "HTTP", host.c_str(), (unsigned)portNumber);
  if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );
  
  updateClient->setTimeout(10000); // 10 sekund timeout
  
  if (!updateClient->connect(host.c_str(), portNumber)) {
    Serial.println("Chyba: Nepodarilo sa pripojit k serveru.");
    if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Chyba: Nepodarilo sa pripojit k serveru." );
    delete updateClient;
    return false;
  }

  // 2) Pošleme HTTP GET požiadavku
  updateClient->printf("GET %s HTTP/1.1\r\n", path.c_str());
  updateClient->printf("Host: %s\r\n", host.c_str());
  updateClient->println("Connection: close");
  updateClient->println("User-Agent: ESP32_OTA/1.0");
  updateClient->println(); // dôležitý prázdny riadok pre koniec hlavičiek

  // 3) Čítame HTTP status line
  unsigned long timeout = millis();
  while (updateClient->available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Chyba: Timeout - server neodpovedal");
      if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Chyba: Timeout - server neodpovedal" );
      updateClient->stop();
      delete updateClient;
      return false;
    }
  }

  // Prečítame status line: "HTTP/1.1 200 OK"
  String statusLine = updateClient->readStringUntil('\n');
  statusLine.trim();
  Serial.printf("HTTP Status: %s\n", statusLine.c_str());
  snprintf(msg, sizeof(msg),"HTTP Status: %s", statusLine.c_str());
  if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );
  
  if (!statusLine.startsWith("HTTP/1.1 200") && !statusLine.startsWith("HTTP/1.0 200")) {
    Serial.printf("Chyba: Server vratil chybny status: %s\n", statusLine.c_str());
    snprintf(msg, sizeof(msg),"Chyba: Server vratil chybny status: %s", statusLine.c_str());
    if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );
    updateClient->stop();
    delete updateClient;
    return false;
  }

  // 4) Čítame HTTP hlavičky a hľadáme Content-Length
  int contentLength = 0;
  bool isValidContentType = false;

  while (updateClient->connected()) {
    String headerLine = updateClient->readStringUntil('\n');
    headerLine.trim();
    
    if (headerLine.length() == 0) {
      // Hlavičky skončili (prázdny riadok)
      break;
    }
    
    Serial.printf("Header: %s\n", headerLine.c_str());
    snprintf(msg, sizeof(msg),"Header: %s", headerLine.c_str());
    if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );

    // Case-insensitive porovnanie pre Content-Length
    String headerLower = headerLine;
    headerLower.toLowerCase();
    
    if (headerLower.startsWith("content-length: ")) {
      contentLength = headerLine.substring(16).toInt();
    }
    
    // Content-Type - akceptujeme rôzne typy
    if (headerLower.startsWith("content-type: ")) {
      String contentType = headerLine.substring(14);
      contentType.trim();
      contentType.toLowerCase();
      // Akceptujeme rôzne content-type pre binárne súbory
      if (contentType.indexOf("octet-stream") >= 0 || 
          contentType.indexOf("bin") >= 0 ||
          contentType.indexOf("application") >= 0) {
        isValidContentType = true;
      }
    }
  }

  Serial.printf("Content-Length: %d, Content-Type OK: %s\n", contentLength, isValidContentType ? "ANO" : "NIE");
  snprintf(msg, sizeof(msg),"Content-Length: %d, Content-Type OK: %s", contentLength, isValidContentType ? "ANO" : "NIE");
  if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );

  // Content-Type kontrola je voliteľná - niektoré servery ju neposielajú
  if (!contentLength) {
    Serial.println("Chyba: Chybajuci Content-Length!");
    if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Chyba: Chybajuci Content-Length!" );
    updateClient->stop();
    delete updateClient;
    return false;
  }

  // 5) Inicializujeme Update
  if (!Update.begin(contentLength)) {
    Serial.printf("Chyba: Update.begin() zlyhalo. Error: %d\n", Update.getError());
    snprintf(msg, sizeof(msg),"Chyba: Update.begin() zlyhalo. Error: %d", Update.getError());
    if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );
    Update.printError(Serial);
    updateClient->stop();
    delete updateClient;
    return false;
  }

  // 6) Čítame telo odpovede a zapisujeme do flash
  Serial.println("Zacinam stahovanie a zapis do flash...");
  if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Zacinam stahovanie a zapis do flash..." );
  const size_t bufferSize = 1024;
  uint8_t buff[bufferSize];
  
  size_t written = 0;
  unsigned long lastDataMillis = millis();
  int progressPercent = 0;
  
  while ((updateClient->connected() || updateClient->available()) && (written < contentLength)) {
    // Aktuálny progress
    int newPercent = (written * 100) / contentLength;
    if (newPercent != progressPercent && newPercent % 10 == 0) {
      progressPercent = newPercent;
      Serial.printf("Progress: %d%% (%d/%d bajtov)\n", progressPercent, written, contentLength);
      snprintf(msg, sizeof(msg),"Progress: %d%% (%d/%d bajtov)", progressPercent, written, contentLength);
      if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );
    }
    
    size_t available = updateClient->available();
    if (available) {
      size_t toRead = (available > bufferSize) ? bufferSize : available;
      int len = updateClient->read(buff, toRead);
      
      if (len > 0) {
        // Úspešne sme prečítali dáta
        size_t bytesWritten = Update.write(buff, len);
        if (bytesWritten != len) {
          Serial.printf("Chyba: Zapisalo sa len %d z %d bajtov\n", bytesWritten, len);
          break;
        }
        written += len;
        lastDataMillis = millis();
      }
    } else {
      // Žiadne dostupné dáta
      if (millis() - lastDataMillis > 10000) {
        // Ak 10 sekúnd nič neprišlo, timeout
        Serial.println("Chyba: Timeout - ziadne data 10 sekund");
        if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Chyba: Timeout - ziadne data 10 sekund" );
        break;
      }
      delay(10); // Krátky oddych
    }

    // Reset watchdogu, ak treba
    if (millis() - last_dtw_millis >= 5000) {
      esp_task_wdt_reset();
      last_dtw_millis = millis();
    }
  }

  updateClient->stop();
  
  // Kontrola, či sme stiahli všetko
  if (written != contentLength) {
    Serial.printf("Chyba: Stiahlo sa len %d z %d bajtov!\n", written, contentLength);
    snprintf(msg, sizeof(msg),"Chyba: Stiahlo sa len %d z %d bajtov!", written, contentLength);
    if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );
    Update.abort();
    delete updateClient;
    return false;
  }

  // 7) Dokončíme Update
  if (Update.end(true)) { // true = nastaví boot partíciu
    Serial.printf("Update end. Celkovo zapisanych: %d bajtov\n", written);
    if (Update.isFinished()) {
      Serial.println("OTA uspesna! Restartujem ESP za 2 sekundy...");
      if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "OTA uspesna! Restartujem ESP za 2 sekundy..." );
      delete updateClient;
      delay(2000);
      ESP.restart();
      return true;
    } else {
      Serial.println("Chyba: Update nie je dokonceny.");
      if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), "Chyba: Update nie je dokonceny." );
      delete updateClient;
      return false;
    }
  } else {
    Serial.printf("Chyba: Update.end() zlyhalo. Error #: %d\n", Update.getError());
    Update.printError(Serial);
    snprintf(msg, sizeof(msg),"Chyba: Update.end() zlyhalo. Error #: %d", Update.getError());
    if (request_from == "mqtt") mqtt.publish(nazov_odosielacieho_kanala.c_str(), msg );
    delete updateClient;
    return false;
  }
}