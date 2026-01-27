/* -----------------------------------------------
*  Citacka GET json prikazov z API servera
*  http://127.0.0.1:9090/api?request={"a":"ping"}
* ------------------------------------------------ */
void apiHandleGetRequest() {
  if (!server.hasArg("request")) {
    server.send(400, "application/json", "{\"error\":\"Missing 'request' parameter\"}");
    return;
  }

  String rawRequest = server.arg("request");             // %7B%22a%22%3A%22ping%22
  String requestStr = html_decode(rawRequest);           // → {"a":"ping"}

  DynamicJsonDocument doc(requestStr.length() * 1.2);
  DeserializationError error = deserializeJson(doc, requestStr);

  if (error) {
    String errResp = "{\"error\":\"Invalid JSON: ";
    errResp += error.c_str();
    errResp += "\"}";
    server.send(400, "application/json", errResp);
    return;
  }

  // Volanie spracovania JSON
  String result = handleJson(doc);

  // Kontrola či result obsahuje chybu
  if (result.startsWith("error:")) {
    DynamicJsonDocument outDoc(result.length() * 1.2 + 50);
    outDoc["error"] = result;
    String response;
    serializeJson(outDoc, response);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "application/json", response);
    return;
  }

  // Výstup: zabaliť ako {"result": ...}
  DynamicJsonDocument outDoc(result.length() * 1.2 + 50); // rezerva na obalenie
  outDoc["result"] = result;

  String response;
  serializeJson(outDoc, response);

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}


/* -----------------------------------------------
*  Citacka POST json prikazov ako body raw
*  http://127.0.0.1:9090/api
* ------------------------------------------------ */
void apiHandlePostRequest() {
  String body = server.arg("plain");

  DynamicJsonDocument doc(body.length() * 1.2);
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  String result = handleJson(doc);
  
  // Kontrola či result obsahuje chybu
  if (result.startsWith("error:")) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "application/json", "{\"error\":\"" + result + "\"}");
    return;
  }
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"result\":" + result + "}");
}

/*
* nastavenie viacerych parametrov v 1 kroku
*  treba to HTML encodovat 
* http://192.168.1.1/setParams?params={"myIPAddress":"192.168.2.2","myGateway":"10.16.0.1","mySubnet":"255.255.255.0","useDHCP":"1"}
* use DHCP bude uloze v tvare String "1" "0"
* IP adresy su ulozene vo formate Strin v bodkovom formate: 192.168.2.2
*/
void setParams() {
  if (server.hasArg("params")) {  // Skontrolujeme, či máme argument "params"
    String jsonStr = server.arg("params");  // Získame hodnotu parametra (JSON reťazec)
    StaticJsonDocument<1024> doc; 
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (error) {
      server.send(400, "text/plain", "Chyba: Neplatný JSON formát.");
      return;
    }
    
      preferences.begin("nastavenia", false);
      for (JsonPair kv : doc.as<JsonObject>()) { // Prejdeme všetky položky JSON objektu
        String paramName = kv.key().c_str();
        String paramValue = kv.value().as<String>();
        preferences.putString(paramName.c_str(), paramValue);
        nastav_globalnu_premennu(paramName, paramValue); //este si aktualizujeme 
      }
      preferences.end();
     
    // Odpoveď serveru
    getParams();
    
  } else {
    server.send(400, "text/plain", "Chyba: Chýba parameter 'params'.");
  }
  
}

/* -----------------------------------------------
*  ziskanie vsetkych parametrov z nastavenia.h z flash pamate
* http://192.168.1.1/getParams
* ------------------------------------------------ */
void getParams(){
  String payload = buildJsonConfig();
  Serial.printf("[webServerTask] Posielam hodnotu: %s\n", payload.c_str());
  server.send(200, "text/json", String(payload) );
}

/**
* @brief Funkcia na odoslanie get reuestu
* @param dotaz // String obsahujuci kompletny dotaz na API
* @return String odpovede zo servera
*/
String odosli_api(String dotaz) {
  WiFiClient client;

  // Parsování URL
  int protocolEnd = dotaz.indexOf("://");
  int hostStart = protocolEnd + 3;
  int portStart = dotaz.indexOf(':', hostStart);
  int pathStart = dotaz.indexOf('/', hostStart);
  
  String host;
  int port = 80; // Predvolený HTTP port

  if (portStart != -1 && portStart < pathStart) {
    // Port je explicitne uvedený v URL
    host = dotaz.substring(hostStart, portStart);
    port = dotaz.substring(portStart + 1, pathStart).toInt();
  } else {
    // Port nie je uvedený, predvolený 80
    host = dotaz.substring(hostStart, pathStart);
  }

  String path = dotaz.substring(pathStart);

  Serial.print("host: ");  Serial.println(host.c_str());
  Serial.print("path: ");  Serial.println(path.c_str());

  unsigned long startMillis = millis();  // Uložení startovního času
  unsigned long timeout = timeout_kontroly_api;  // Timeout 5 sekund

  // Připojení k serveru s pokusy o opětovné připojení
  bool connected = false;
  for (int i = 0; i < 3; i++) {
    if (client.connect(host.c_str(), port)) {
      connected = true;
      break;
    }
    delay(500); // Pauza mezi pokusy
  }

  if (!connected) {
    Serial.printf("Connection to host failed after multiple attempts\n");
    return "NOK";
  }
  Serial.printf("Odosielam: %s \n", dotaz.c_str() );

 // Vytvoření GET požadavku s hlavičkami
  client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n" +
               "User-Agent: ESP32Client\r\n" +
               "Accept: */*\r\n" +
               "\r\n");

  while (client.connected() && (millis() - startMillis < timeout)) {
    if (client.available()) {
      break;
    }
    if (millis() - startMillis >= timeout) {
      Serial.printf("API timeout reached, no response from server.\n");
      client.stop();
      return "NOK";
    }
  }

  // Čtení těla odpovědi
  String response = "";
  while (client.available()) {
    response += client.readString();
  }
  client.stop();
  
  if (response == "") {
    Serial.printf("Empty response from server. \n");
    return "NOK";
  }

  // Extrakce JSON části z odpovědi
  int jsonStart = response.indexOf('{');
  int jsonEnd = response.lastIndexOf('}') + 1;
  if (jsonStart != -1 && jsonEnd != -1) {
    response = response.substring(jsonStart, jsonEnd);
  } else {
    Serial.printf("Invalid JSON response. \n");
    return "NOK";
  }

  return response;
}



