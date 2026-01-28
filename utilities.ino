/**
 * fcia na poskytovanie informacii LEN o branach (gate_status)
 */
String get_gate_status(){

  StaticJsonDocument<512> doc;
  doc["current_millis"]  = currentMillis;

  JsonObject gateStatus = doc.createNestedObject("gate_status");
  for (const auto& [pin, _] : koniec) {
    const char* stav = (pinStav[pin] == GATE_UP) ? "GATE_UP" : "GATE_DOWN";
    
    // Použiť textový názov namiesto čísla pinu
    String gateName;
    switch (pin) {
      case GATE1: gateName = "GATE1"; break;
      case GATE2: gateName = "GATE2"; break;
      case GATE3: gateName = "GATE3"; break;
      case GATE4: gateName = "GATE4"; break;
      default: gateName = String(pin); break; // Fallback pre neznáme piny
    }
    
    gateStatus[gateName] = stav;
  }

  JsonObject gateOffTime = doc.createNestedObject("gate_off_time");
  for (const auto& [pin, offTime] : koniec) {
    // Použiť textový názov namiesto čísla pinu
    String gateName;
    switch (pin) {
      case GATE1: gateName = "GATE1"; break;
      case GATE2: gateName = "GATE2"; break;
      case GATE3: gateName = "GATE3"; break;
      case GATE4: gateName = "GATE4"; break;
      default: gateName = String(pin); break; // Fallback pre neznáme piny
    }
    
    gateOffTime[gateName] = offTime;
  }

  String output;
  serializeJson(doc, output);
  if (verbose) {
    Serial.println(output);
  }
  return output;
}

/**
 * fcia na poskytovanie informacii
 */
String get_info(){

  StaticJsonDocument<1024> doc;
  doc["my_hostname"]     = ETH.getHostname();
  doc["my_mac_address"]  = ETH.macAddress();
  doc["my_ip_address"]   = ETH.localIP().toString();
  doc["uuid"]            = uuid;
  doc["firmware"]        = version;
  doc["is_mqtt_allowed"] = is_mqtt_allowed;
  doc["current_millis"]  = currentMillis;

  JsonObject gateStatus = doc.createNestedObject("gate_status");
  for (const auto& [pin, _] : koniec) {
    const char* stav = (pinStav[pin] == GATE_UP) ? "GATE_UP" : "GATE_DOWN";
    
    // Použiť textový názov namiesto čísla pinu
    String gateName;
    switch (pin) {
      case GATE1: gateName = "GATE1"; break;
      case GATE2: gateName = "GATE2"; break;
      case GATE3: gateName = "GATE3"; break;
      case GATE4: gateName = "GATE4"; break;
      default: gateName = String(pin); break; // Fallback pre neznáme piny
    }
    
    gateStatus[gateName] = stav;
  }

  JsonObject gateOffTime = doc.createNestedObject("gate_off_time");
  for (const auto& [pin, offTime] : koniec) {
    // Použiť textový názov namiesto čísla pinu
    String gateName;
    switch (pin) {
      case GATE1: gateName = "GATE1"; break;
      case GATE2: gateName = "GATE2"; break;
      case GATE3: gateName = "GATE3"; break;
      case GATE4: gateName = "GATE4"; break;
      default: gateName = String(pin); break; // Fallback pre neznáme piny
    }
    
    gateOffTime[gateName] = offTime;
  }

  String output;
  serializeJson(doc, output);
  if (verbose) {
    Serial.println(output);
  }
  return output;
}

/**
 * fcia nastavenia flash pamete nekonecneho stavu relatok
 */
void nastav_stav_flash_gate(unsigned int gpio, bool stav){
  preferences.begin("gate_status");
  switch (gpio) {
    case GATE1:
      preferences.putBool("gate1_status", stav);
      break;
    case GATE2:
      preferences.putBool("gate2_status", stav);
      break;
    case GATE3:
      preferences.putBool("gate3_status", stav);
      break;
    case GATE4:
      preferences.putBool("gate4_status", stav);
      break;
  }
  preferences.end();
}

/**
**  fcia ktora v jsone vypise na konzolu vlatnu identifikaciu
**  identifikacia: uuid - vlasten mac adresa
**  po prvom spusteni sa vygeneruje uuid a pin a nasledne sa ulozi do Flash
**/
void say_hello() {
  if (uuid == "") {                    // este nemame, ideme generovat
    uuid = String(ESP.getEfuseMac());  // ziskanie id chipu
    preferences.begin("my_id");
    preferences.putString("uuid", uuid);
    preferences.end();
  }

  //vypis info na konzolu, uvitacia sprava
  Serial.printf("{\"action\":\"hello\",\"uuid\":\"%s\", \"version\":\"%s\"} \n", uuid.c_str(), version.c_str());
}
  
/*   --------------------------------
**  prebehne flash a ponastavuje premenne Preferences
*   --------------------------------  */
void proceess_preferences(){
  preferences.begin("nastavenia");
    my_hostname = preferences.getString("my_hostname", my_hostname);
    broker = preferences.getString("broker", broker);
    mqtt_port = preferences.getUInt("mqtt_port", mqtt_port);
    mqtt_username = preferences.getString("mqtt_username", mqtt_username);
    mqtt_password = preferences.getString("mqtt_password", mqtt_password);
    is_mqtt_allowed = preferences.getUInt("is_mqtt_allowed", is_mqtt_allowed);
    nazov_clienta = preferences.getString("nazov_clienta", nazov_clienta);
    useDHCP = preferences.getBool("useDHCP", useDHCP);
    verbose = preferences.getBool("verbose", verbose);
    
    // Načítanie IP adries zo stringu
    String ipStr = preferences.getString("myIPAddress", "");
    if (ipStr.length() > 0) { myIPAddress.fromString(ipStr); }
    ipStr = preferences.getString("myGateway", "");
    if (ipStr.length() > 0) { myGateway.fromString(ipStr); }
    ipStr = preferences.getString("mySubnet", "");
    if (ipStr.length() > 0) { mySubnet.fromString(ipStr); }
    ipStr = preferences.getString("myPrimaryDNS", "");
    if (ipStr.length() > 0) { myPrimaryDNS.fromString(ipStr); }
    ipStr = preferences.getString("mySecondaryDNS", "");
    if (ipStr.length() > 0) { mySecondaryDNS.fromString(ipStr); }
    
    Serial.printf("Load from flash my_hostname => %s \n", my_hostname.c_str());
    Serial.printf("Load from flash broker  \n" );
    Serial.printf("Load from flash mqtt_port  \n");
    Serial.printf("Load from flash mqtt_username  \n");
    Serial.printf("Load from flash mqtt_password \n");
    Serial.printf("Load from flash is_mqtt_allowed => %d \n", is_mqtt_allowed);
    Serial.printf("Load from flash nazov_clienta => %s \n", nazov_clienta.c_str() );
    Serial.printf("Load from flash useDHCP => %d \n", useDHCP);
    Serial.printf("Load from flash verbose => %d \n", verbose);
    Serial.printf("Load from flash myIPAddress => %s \n", myIPAddress.toString().c_str());
    Serial.printf("Load from flash myGateway => %s \n", myGateway.toString().c_str());
    Serial.printf("Load from flash mySubnet => %s \n", mySubnet.toString().c_str());
    Serial.printf("Load from flash myPrimaryDNS => %s \n", myPrimaryDNS.toString().c_str());
    Serial.printf("Load from flash mySecondaryDNS => %s \n", mySecondaryDNS.toString().c_str());
  preferences.end();
}

/*   --------------------------------
**  ZAKODOVANIE normalneho textu do HTML
*   --------------------------------  */
String html_encode(String text) {
  String encoded = "";
  for (int i = 0; i < text.length(); i++) {
    char c = text[i];
    switch (c) {
      case ' ': encoded += "%20"; break;  // Medzera
      case '&': encoded += "%26;"; break;
      case '<': encoded += "%3C"; break;
      case '>': encoded += "%3E"; break;
      case '"': encoded += "%22"; break;
      case '\'': encoded += "%27"; break;
      case '+': encoded += "%2B"; break; // Znak +
      default:
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
          encoded += c; // Bez zmeny pre alfanumerické znaky a bežné znaky
        } else {
          encoded += '%';
          encoded += String(c, HEX); // Iné znaky enkódujeme na hexadecimálne
        }
        break;
    }
  }
  return encoded;
}


/*   --------------------------------
**  ODKODOVANIE HTML textu do normalnych znakov
*   --------------------------------  */
String html_decode(String text) {
  String decoded = text;

  // Dekódovanie HTML entít
  decoded.replace("&amp;", "&");
  decoded.replace("&lt;", "<");
  decoded.replace("&gt;", ">");
  decoded.replace("&quot;", "\"");
  decoded.replace("&#39;", "\'");

  // Dekódovanie znaku '+' na medzeru
  decoded.replace('+', ' ');

  // Dekódovanie percent-enkódovania
  int index = 0;
  while ((index = decoded.indexOf('%', index)) != -1) {
    if (index + 2 < decoded.length()) {
      char hexVal[3] = { decoded[index + 1], decoded[index + 2], '\0' };
      char decodedChar = (char)strtol(hexVal, NULL, 16); // Hex na char
      
      // Zostavíme reťazec manuálne
      decoded = decoded.substring(0, index) + decodedChar + decoded.substring(index + 3);
    }
  }

  return decoded;
}

/*   --------------------------------
*  Nastavenie globalnych premennych 
*   --------------------------------  */
void nastav_globalnu_premennu(String paramName, String paramValue){
  // Aktualizácia globálnych premennych
  if (paramName == "version") {
    version = paramValue;  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota version: " + version);

  } else if ( paramName == "myIPAddress" ){
    IPAddress newIP;
    if (newIP.fromString(paramValue)) {  // Konverzia String na IPAddress
        myIPAddress = newIP;  // Nastavenie globálnej premennej
        Serial.println("IP adresa úspešne nastavená: " + newIP.toString());
    } else { Serial.println("Chyba: Nesprávny formát IP adresy!"); }

  } else if ( paramName == "myGateway" ){
    IPAddress newGW;
    if (newGW.fromString(paramValue)) {  // Konverzia String na IPAddress
        myGateway = newGW;  // Nastavenie globálnej premennej
        Serial.println("GW adresa úspešne nastavená: " + newGW.toString());
    } else{Serial.println("Chyba: Nesprávny formát GW adresy!"); } 

  } else if ( paramName == "mySubnet" ){
    IPAddress newSN;
    if (newSN.fromString(paramValue)) {  // Konverzia String na IPAddress
        mySubnet = newSN;  // Nastavenie globálnej premennej
        Serial.println("SubNet adresa úspešne nastavená: " + newSN.toString());
    } else{Serial.println("Chyba: Nesprávny formát Subnet adresy!"); }

  } else if ( paramName == "myPrimaryDNS" ){
    IPAddress newPDNS;
    if (newPDNS.fromString(paramValue)) {  // Konverzia String na IPAddress
        myPrimaryDNS = newPDNS;  // Nastavenie globálnej premennej
        Serial.println("Primary DNS adresa úspešne nastavená: " + newPDNS.toString());
    } else{Serial.println("Chyba: Nesprávny formát Primary DNS adresy!"); }

  } else if ( paramName == "mySecondaryDNS" ){
    IPAddress newSDNS;
    if (newSDNS.fromString(paramValue)) {  // Konverzia String na IPAddress
        mySecondaryDNS = newSDNS;  // Nastavenie globálnej premennej
        Serial.println("mySecondaryDNS adresa úspešne nastavená: " + newSDNS.toString());
    } else{Serial.println("Chyba: Nesprávny formát mySecondaryDNS adresy!"); }

  } else if ( paramName == "useDHCP" ){
    useDHCP = (paramValue == "1");  // Konverzia String -> bool
    Serial.printf("useDHCP úspešne nastavená: %s \n", useDHCP ? "true" : "false" );

  } else if (paramName == "uuid") {
    uuid = paramValue;  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: uuid: " + uuid);

  } else if (paramName == "broker") {
    broker = paramValue;  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: broker: " + broker);

  }  else if (paramName == "mqtt_port") {
    mqtt_port = paramValue.toInt();  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: mqtt_port: " + mqtt_port);

  } else if (paramName == "mqtt_username") {
    mqtt_username = paramValue;  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: mqtt_username: " + mqtt_username);

  } else if (paramName == "mqtt_password") {
    mqtt_password = paramValue;  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: mqtt_password: " + mqtt_password);

  } else if (paramName == "is_mqtt_allowed") {
    is_mqtt_allowed = paramValue.toInt();  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: is_mqtt_allowed: " + is_mqtt_allowed);

  } else if (paramName == "my_hostname") {
    my_hostname = paramValue;  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: my_hostname: " + my_hostname);

  } else if (paramName == "nazov_clienta") {
    nazov_clienta = paramValue;  // Zmena hodnoty globálnej premennej
    Serial.println("Aktualizovana globalna hodnota: nazov_clienta: " + nazov_clienta);

  } else if (paramName == "verbose") {
    verbose = (paramValue == "1" || paramValue == "true");  // Konverzia String -> bool
    Serial.printf("verbose úspesne nastavená: %s \n", verbose ? "true" : "false" );

  }

}

/*   --------------------------------
*  Ziskanie globalnych premennych 
*   --------------------------------  */
String buildJsonConfig() {
  DynamicJsonDocument getflash_doc(1024);
  JsonArray getflash = getflash_doc.createNestedArray("getflash");

  { JsonObject doc = getflash.createNestedObject();  doc["my_hostname"] = my_hostname;  }
  { JsonObject doc = getflash.createNestedObject();  doc["nazov_clienta"] = nazov_clienta;  }
  { JsonObject doc = getflash.createNestedObject();  doc["broker"] = broker;  }
  { JsonObject doc = getflash.createNestedObject();  doc["mqtt_port"] = mqtt_port;  }
  { JsonObject doc = getflash.createNestedObject();  doc["mqtt_username"] = "*****";  }
  { JsonObject doc = getflash.createNestedObject();  doc["mqtt_password"] = "*****";  }
  { JsonObject doc = getflash.createNestedObject();  doc["is_mqtt_allowed"] = is_mqtt_allowed;  }
  { JsonObject doc = getflash.createNestedObject();  doc["myIPAddress"] = myIPAddress.toString();  }
  { JsonObject doc = getflash.createNestedObject();  doc["myGateway"] = myGateway.toString();  }
  { JsonObject doc = getflash.createNestedObject();  doc["mySubnet"] = mySubnet.toString();  }
  { JsonObject doc = getflash.createNestedObject();  doc["myPrimaryDNS"] = myPrimaryDNS.toString();  }
  { JsonObject doc = getflash.createNestedObject();  doc["mySecondaryDNS"] = mySecondaryDNS.toString();  }
  { JsonObject doc = getflash.createNestedObject();  doc["useDHCP"] = useDHCP ? "1" : "0";  }
  { JsonObject doc = getflash.createNestedObject();  doc["verbose"] = verbose ? "1" : "0";  }

  // Konvertovanie JSON do String
  String json_output;
  serializeJson(getflash_doc, json_output);
  return json_output;
}
