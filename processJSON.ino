//-------------------------------------------------------------------
// ======== Pomocná funkcia na konverziu gate mena na pin ==========
//-------------------------------------------------------------------
// Vráti číslo pinu ak je validné, inak -1
int parseGatePin(JsonVariant gateValue) {
  // Ak je to číslo, vrátime ho priamo
  if (gateValue.is<int>() || gateValue.is<unsigned int>()) {
    return gateValue.as<int>();
  }
  
  // Ak je to string, konvertujeme názov na číslo
  if (gateValue.is<const char*>() || gateValue.is<String>()) {
    String gateName = gateValue.as<String>();
    
    if (gateName == "GATE1") return GATE1;
    if (gateName == "GATE2") return GATE2;
    if (gateName == "GATE3") return GATE3;
    if (gateName == "GATE4") return GATE4;
    
    // Pokus o konverziu ako číslo (pre backwards compatibility)
    int pinNum = gateName.toInt();
    if (pinNum > 0 || gateName == "0") {
      return pinNum;
    }
  }
  
  return -1; // Neplatná hodnota
}

//-------------------------------------------------------------------
// ======== Spoločná funkcia na spracovanie akcií ===================
//-------------------------------------------------------------------
String handleJson(DynamicJsonDocument& doc, const String& request_from) {
  // Kontrola či existuje povinný parameter "a"
  if (!doc.containsKey("a")) {
    Serial.println("Error: Missing 'a' parameter in JSON");
    return "error: missing action parameter";
  }
  
  const char* action = doc["a"];  //definicia retazca action a jeho naplnenie hodnotou jsonu kluca "a"
  
  // Kontrola či action nie je null
  if (action == nullptr || strlen(action) == 0) {
    Serial.println("Error: Empty action parameter");
    return "error: empty action parameter";
  }
  
  Serial.print("Action request received: ");
  Serial.println(action);

  // Extrakcia rid parametra ak existuje
  String rid = "";
  if (doc.containsKey("rid")) {
    rid = doc["rid"].as<String>();
    Serial.print("RID received: ");
    Serial.println(rid);
  }

  String result = ""; // vysledok spracovania jsonu

  // rozcestnik na prijate spravy
  /**
   *  prijatie resetovania dosky
   */
  if (strcmp(action, "reset") == 0) { 
    Serial.println("Resetting ESP...");
    mqtt.publish(nazov_odosielacieho_kanala.c_str(), "resetSuccess"); // odpovemie do kanal preddefinovanou odpoved
    delay(200);                                                       // tu si hadam mozem dovolit blocking delay
    ESP.restart();
  }
      /**
     *   manipulacia s RELATKAMI
     *  {  "a":"gate", "g": "4",  "d": "5"} -> zapnutie, lebo predlzenie portu na 5 sek odteraz
     *  {  "a":"gate", "g": "4",  "d": "5",  "min_d":"10" } -> zapnutie portu na minimalne min_d odteraz, pri VYPNUTOM porte
     *  {  "a":"gate", "g": "4",  "d": "5",  "min_d":"10"} -> predlzenie zapnutia portu na dalsich cas "d" odteraz ale od hranice min_d
     *  {  "a":"gate", "g": "4",  "d": "0" } -> okamzite vypnutie portu
     *  {  "a":"gate", "g": "4",  "d": "11111" } -> zapnutie portu na nekonecne vela
     */
  else if (strcmp(action, "gate") == 0) { 
    if(!(doc["g"] && doc["d"])){ //povinne parametre
      return rid.length() > 0 ? "{\"result\":\"action gate failed\",\"rid\":\"" + rid + "\"}" : "action gate failed";  //odpovieme do kanala preddefinovanou odpoved
    }

    // Parsovanie GPIO - podporuje aj textové názvy (GATE1, GATE2, ...)
    int gpio = parseGatePin(doc["g"]);
    
    // Kontrola ci je GPIO v rozsahu povolenych pinov
    if (gpio == -1 || (gpio != GATE1 && gpio != GATE2 && gpio != GATE3 && gpio != GATE4)) {
      Serial.print("Neplatny GPIO pin: ");
      Serial.println(gpio);
      return rid.length() > 0 ? "{\"result\":\"Invalid GPIO pin\",\"rid\":\"" + rid + "\"}" : "Invalid GPIO pin";
    }
    
    unsigned int duration = (unsigned int) doc["d"];
    const char* respond_to = doc["r"]; //pokus o nacitanie poziadavky na priamu odpoved
    String odpoved = "";
    
    //ak je dany parameter min_d && 
    if (doc["min_d"] && (pinStav[gpio] == GATE_DOWN))
      duration = (unsigned int) doc["min_d"];

    /*****  vypnutie portu ak je trvanie nastavene na 0 *******/
    if (duration == 0)  {
      Serial.print("Vypinam port:");
      Serial.println(gpio);
      odpoved = vypni(gpio);
      nastav_stav_flash_gate(gpio, false); //nastavenie flash pamate
    }
    /***** trvale zapnutie portu *****/
    else if ( ((unsigned int) doc["d"]) == 11111){ 
      Serial.print("ZAPINAM port NEKONECNE:");  Serial.println(gpio);
      odpoved = zapni_endless(gpio); 
      nastav_stav_flash_gate(gpio, true); //nastavenie flash pamate
    } else  { /*  hodnota duration nieje 0, ZAPINAME PORT    */
      Serial.print("ZAPINAM port:");  Serial.println(gpio);
      odpoved = zapni(gpio, duration);
      
    }
    Serial.print("[jsonProsess]:"); Serial.print(gpio);    Serial.print(":");     Serial.println(duration);
    return rid.length() > 0 ? "{\"result\":\"" + odpoved + "\",\"rid\":\"" + rid + "\"}" : odpoved;
  }

       /**
     *   manipulacia s RELATKAMI - MILISEKUNDY
     *  {  "a":"gate_ms", "g": "4",  "d": "3000",  "min_d":"10000" } -> zapnutie portu na minimalne min_d odteraz, pri VYPNUTOM porte
     *  {  "a":"gate_ms", "g": "4",  "d": "3000",  "min_d":"10000"} -> predlzenie zapnutia portu na dalsich cas "d" odteraz ale od hranice min_d
     *  {  "a":"gate_ms", "g": "4",  "d": "0" } -> okamzite vypnutie portu
     *  {  "a":"gate_ms", "g": "14",  "d": "11111" } -> zapnutie portu na nekonecne vela
     */
  else if (strcmp(action, "gate_ms") == 0) { 
    if(!(doc["g"] && doc["d"])){ //povinne parametre
      return rid.length() > 0 ? "{\"result\":\"action gate failed\",\"rid\":\"" + rid + "\"}" : "action gate failed";  //odpovieme do kanala preddefinovanou odpoved
    }
    
    // Parsovanie GPIO - podporuje aj textové názvy (GATE1, GATE2, ...)
    int gpio = parseGatePin(doc["g"]);
    
    // Kontrola ci je GPIO v rozsahu povolenych pinov
    if (gpio == -1 || (gpio != GATE1 && gpio != GATE2 && gpio != GATE3 && gpio != GATE4)) {
      Serial.print("Neplatny GPIO pin: ");
      Serial.println(gpio);
      return rid.length() > 0 ? "{\"result\":\"Invalid GPIO pin\",\"rid\":\"" + rid + "\"}" : "Invalid GPIO pin";
    }
    
    unsigned int duration = (unsigned int) doc["d"];
    const char* respond_to = doc["r"]; //pokus o nacitanie poziadavky na priamu odpoved
    String odpoved = "";

    //ak je dany parameter min_d && 
    if (doc["min_d"] && (pinStav[gpio] == GATE_DOWN))
      duration = (unsigned int) doc["min_d"];

    /*****  vypnutie portu ak je trvanie nastavene na 0 *******/
    if (duration == 0)  {
      Serial.print("Vypinam port:");
      Serial.println(gpio);
      odpoved = vypni(gpio);
      nastav_stav_flash_gate(gpio, false); //nastavenie flash pamate
    }
    /***** trvale zapnutie portu *****/
    else if ( ((unsigned int) doc["d"]) == 11111){ 
      Serial.print("ZAPINAM port NEKONECNE:");  Serial.println(gpio);
      odpoved = zapni_endless(gpio); 
      nastav_stav_flash_gate(gpio, true); //nastavenie flash pamate
    } else  { /*  hodnota duration nieje 0, ZAPINAME PORT    */
      Serial.print("ZAPINAM port:");  Serial.println(gpio);
      odpoved = zapni(gpio, duration, true); //zapnutie v miliseknudach
    }
    Serial.print("[jsonProsess]:"); Serial.print(gpio);    Serial.print(":");     Serial.println(duration);
    return rid.length() > 0 ? "{\"result\":\"" + odpoved + "\",\"rid\":\"" + rid + "\"}" : odpoved;
  }

  /**
  *  pingnutie dosky, zistenie jej pritomnosti
  */
  else if (strcmp(action, "ping") == 0) { 
    return rid.length() > 0 ? "{\"result\":\"pingOK\",\"rid\":\"" + rid + "\"}" : "pingOK";
  }
  
  /**
   *  poziadavka na poslatie stavu zariadenia
   */
  else if( strcmp(action,"status") == 0  ){ /* akcia manipulacie s portami */
    String statusResult = get_info();
    return rid.length() > 0 ? "{\"result\":" + statusResult + ",\"rid\":\"" + rid + "\"}" : statusResult; 
  }

  /**
   *  poziadavka na ziskanie LEN stavov bran (gate_status)
   * {  "a":"gate_status" }
   */
  else if (strcmp(action, "gate_status") == 0) {
    String gateStatusResult = get_gate_status();
    return rid.length() > 0 ? "{\"result\":" + gateStatusResult + ",\"rid\":\"" + rid + "\"}" : gateStatusResult;
  }

  /**
   *  poziadavka na nastavenie parametrov flash pamate
   * {  "a":"setflash", "premenna": "hodnota" }
   */
  else if( strcmp(action,"setflash") == 0  ){ 
    String setflashResult = setflash(doc);
    return rid.length() > 0 ? "{\"result\":\"" + setflashResult + "\",\"rid\":\"" + rid + "\"}" : setflashResult;
  }
  /**
   *  poziadavka na ziskanie prametrov z flash pamate
    {  "a":"getflash" }
   */
  else if( strcmp(action,"getflash") == 0  ){ 
    String getflashResult = getflash();
    return rid.length() > 0 ? "{\"result\":" + getflashResult + ",\"rid\":\"" + rid + "\"}" : getflashResult;
  }

  /**
   *  poziadavka na help
    {  "a":"help" }
   */
  else if( strcmp(action,"help") == 0  ){ 
    String helpResult = send_help();
    return rid.length() > 0 ? "{\"result\":" + helpResult + ",\"rid\":\"" + rid + "\"}" : helpResult;
  }

  /**
  *  poziadavka na upate firmware, bezi na SSL vrstve!
  *  update pojde podla parametrov priamo z prisleho mqtt prikazu
  * {  "a":"make_update", "ota_host": "192.168.1.1", "ota_port" : "443", "ota_path": "/firmwares/vaha.bin"}
  * ota_port je nepovinny, pokial nebude uvedeny berie sa 443
  */
  else if( strcmp(action,"make_update") == 0  ){ 
    Serial.println("Poziadavka na UPDATE softwaru s parametrami:");
    
    // Debug - vypis prijateho JSONu
    Serial.printf("Kontrola klucov: ota_host=%s, ota_path=%s, ota_port=%s\n",
      doc.containsKey("ota_host") ? "ANO" : "NIE",
      doc.containsKey("ota_path") ? "ANO" : "NIE",
      doc.containsKey("ota_port") ? "ANO" : "NIE");
    
    if(doc.containsKey("ota_host") && doc.containsKey("ota_path")){
      String tmp_ota_host = doc["ota_host"].as<String>();
      String tmp_ota_port = doc.containsKey("ota_port") ? doc["ota_port"].as<String>() : "443";
      String tmp_ota_path = doc["ota_path"].as<String>();
      
      Serial.printf("Hodnoty: host=%s, port=%s, path=%s\n", 
        tmp_ota_host.c_str(), tmp_ota_port.c_str(), tmp_ota_path.c_str());
      
      // Kontrola ci je ota_port cislo
      int port_num = tmp_ota_port.toInt();
      if (port_num <= 0 || port_num > 65535) {
        Serial.println("Chyba: ota_port musi byt cislo v rozsahu 1-65535");
        return rid.length() > 0 ? "{\"result\":\"Chyba: ota_port musi byt cislo v rozsahu 1-65535\",\"rid\":\"" + rid + "\"}" : "Chyba: ota_port musi byt cislo v rozsahu 1-65535";
      }
      
      performOTAUpdate(tmp_ota_host, tmp_ota_port, tmp_ota_path, request_from);
      return rid.length() > 0 ? "{\"result\":\"Spracovanie OTA ukoncene\",\"rid\":\"" + rid + "\"}" : "Spracovanie OTA ukoncene";
    } else {
      Serial.println("Chybajuce, alebo neplatne parametre pre OTA");  
      return rid.length() > 0 ? "{\"result\":\"Chybajuce, alebo neplatne parametre pre OTA\",\"rid\":\"" + rid + "\"}" : "Chybajuce, alebo neplatne parametre pre OTA";
    }
    
  }
  
  /**
   *  zapnutie verbose mode (debug výpisy)
   * {  "a":"verbose_on" }
   */
  else if (strcmp(action, "verbose_on") == 0) {
    verbose = true;
    preferences.begin("nastavenia");
    preferences.putBool("verbose", verbose);
    preferences.end();
    Serial.println("Verbose mode ENABLED");
    return rid.length() > 0 ? "{\"result\":\"verbose enabled\",\"rid\":\"" + rid + "\"}" : "verbose enabled";
  }
  
  /**
   *  vypnutie verbose mode (debug výpisy)
   * {  "a":"verbose_off" }
   */
  else if (strcmp(action, "verbose_off") == 0) {
    verbose = false;
    preferences.begin("nastavenia");
    preferences.putBool("verbose", verbose);
    preferences.end();
    Serial.println("Verbose mode DISABLED");
    return rid.length() > 0 ? "{\"result\":\"verbose disabled\",\"rid\":\"" + rid + "\"}" : "verbose disabled";
  }

  /**
   *  zapnutie vpouzivania mqtt
   * {  "a":"mqtt_on" }
   */
  else if (strcmp(action, "mqtt_on") == 0) {
    is_mqtt_allowed = 1;
    preferences.begin("nastavenia", false);
      preferences.putUInt("is_mqtt_allowed", is_mqtt_allowed);
    preferences.end();
    if (is_mqtt_allowed == 0) { stav.mqtt=2; updateLedFromStav(); mqtt.disconnect(); }
    if (is_mqtt_allowed == 1) { stav.mqtt=0; updateLedFromStav(); }
    Serial.println("MQTT mode ENABLED");
    return rid.length() > 0 ? "{\"result\":\"mqtt enabled\",\"rid\":\"" + rid + "\"}" : "mqtt enabled";
  }

  /**
   *  vypnutie vpouzivania mqtt
   * {  "a":"mqtt_off" }
   */
  else if (strcmp(action, "mqtt_off") == 0) {
    is_mqtt_allowed = 0;
    preferences.begin("nastavenia", false);
      preferences.putUInt("is_mqtt_allowed", is_mqtt_allowed);
    preferences.end();
    if (is_mqtt_allowed == 0) { stav.mqtt=2; updateLedFromStav(); mqtt.disconnect(); }
    if (is_mqtt_allowed == 1) { stav.mqtt=0; updateLedFromStav(); }
    Serial.println("MQTT mode DISABLED");
    return rid.length() > 0 ? "{\"result\":\"mqtt disabled\",\"rid\":\"" + rid + "\"}" : "mqtt disabled";
  }
  
  /**
    *  nepozname taky prikaz
  */
  else  { // vyhodime error, pretoze nerozumieme sprave
    return rid.length() > 0 ? "{\"result\":\"unknown request\",\"rid\":\"" + rid + "\"}" : "unknown request";
  } // end rozcestnik na prijate spravy
}

/******************************************************************************************
   *  poziadavka na nastavenie parametrov flash pamate
   * {  "a":"setflash", "premenna": "hodnota" }
   */
String setflash(DynamicJsonDocument doc){
  String navrat = "Toto sa nastavilo: ";

  if(doc["my_hostname"]){ //este kontrola ci je to retazec
    my_hostname = doc["my_hostname"].as<String>();
    //nastavenie flash pamate
    preferences.begin("nastavenia", false);
      preferences.putString("my_hostname", my_hostname );//ulozenie udaja do flash
    preferences.end();
    //este to posleme do slaveov
    navrat += " my_hostname";
  }

  if(doc["broker"]){ //este kontrola ci je to retazec
    broker = doc["broker"].as<String>();
    //nastavenie flash pamate
    preferences.begin("nastavenia", false);
      preferences.putString("broker", broker );//ulozenie udaja do flash
    preferences.end();
    //este to posleme do slaveov
    navrat += " broker";
  }

  if(doc["mqtt_port"]){ //este kontrola ci je to retazec
    mqtt_port = doc["mqtt_port"].as<unsigned int>();
    //nastavenie flash pamate
    preferences.begin("nastavenia", false);
      preferences.putUInt("mqtt_port", mqtt_port );//ulozenie udaja do flash
    preferences.end();
    //este to posleme do slaveov
    navrat += " mqtt_port";
  }

  if(doc["mqtt_username"]){ //este kontrola ci je to retazec
    mqtt_username = doc["mqtt_username"].as<String>();
    //nastavenie flash pamate
    preferences.begin("nastavenia", false);
      preferences.putString("mqtt_username", mqtt_username );//ulozenie udaja do flash
    preferences.end();
    //este to posleme do slaveov
    navrat += " mqtt_username";
  }

  if(doc["mqtt_password"]){ //este kontrola ci je to retazec
    mqtt_password = doc["mqtt_password"].as<String>();
    //nastavenie flash pamate
    preferences.begin("nastavenia", false);
      preferences.putString("mqtt_password", mqtt_password );//ulozenie udaja do flash
    preferences.end();
    //este to posleme do slaveov
    navrat += " mqtt_password";
  }

  if(doc["nazov_clienta"]){ //este kontrola ci je to retazec
    nazov_clienta = doc["nazov_clienta"].as<String>();
    //nastavenie flash pamate
    preferences.begin("nastavenia", false);
      preferences.putString("nazov_clienta", nazov_clienta );//ulozenie udaja do flash
    preferences.end();
    //este to posleme do slaveov
    navrat += " nazov_clienta";
  }

  if (doc.containsKey("is_mqtt_allowed")) {
    uint32_t val = doc["is_mqtt_allowed"].as<uint32_t>();  // prečíta aj stringove cislo
    if (val == 0 || val == 1) {  // len tieto dve hodnoty
      is_mqtt_allowed = val;
      preferences.begin("nastavenia", false);
      preferences.putUInt("is_mqtt_allowed", is_mqtt_allowed);
      preferences.end();
      if (val == 0) { stav.mqtt=2; updateLedFromStav(); mqtt.disconnect(); }
      if (val == 1) { stav.mqtt=0; updateLedFromStav(); }
      navrat += " is_mqtt_allowed";
    } else {
      navrat += " !is_mqtt_allowed neplatna hodnota!";
    }
  }

  if (doc.containsKey("useDHCP")) {
    uint32_t val = doc["useDHCP"].as<uint32_t>();  // prečíta aj stringove cislo
    if (val == 0 || val == 1) {  // len tieto dve hodnoty
      useDHCP = (val == 1);
      preferences.begin("nastavenia", false);
      preferences.putBool("useDHCP", useDHCP);
      preferences.end();
      navrat += " useDHCP";
    } else {
      navrat += " !useDHCP neplatna hodnota!";
    }
  }

  if(doc["myIPAddress"]){
    String ipStr = doc["myIPAddress"].as<String>();
    IPAddress tempIP;
    if (tempIP.fromString(ipStr)) {
      myIPAddress = tempIP;
      preferences.begin("nastavenia", false);
      preferences.putString("myIPAddress", myIPAddress.toString());
      preferences.end();
      navrat += " myIPAddress";
    } else {
      navrat += " !myIPAddress neplatny format!";
    }
  }

  if(doc["myGateway"]){
    String ipStr = doc["myGateway"].as<String>();
    IPAddress tempIP;
    if (tempIP.fromString(ipStr)) {
      myGateway = tempIP;
      preferences.begin("nastavenia", false);
      preferences.putString("myGateway", myGateway.toString());
      preferences.end();
      navrat += " myGateway";
    } else {
      navrat += " !myGateway neplatny format!";
    }
  }

  if(doc["mySubnet"]){
    String ipStr = doc["mySubnet"].as<String>();
    IPAddress tempIP;
    if (tempIP.fromString(ipStr)) {
      mySubnet = tempIP;
      preferences.begin("nastavenia", false);
      preferences.putString("mySubnet", mySubnet.toString());
      preferences.end();
      navrat += " mySubnet";
    } else {
      navrat += " !mySubnet neplatny format!";
    }
  }

  if(doc["myPrimaryDNS"]){
    String ipStr = doc["myPrimaryDNS"].as<String>();
    IPAddress tempIP;
    if (tempIP.fromString(ipStr)) {
      myPrimaryDNS = tempIP;
      preferences.begin("nastavenia", false);
      preferences.putString("myPrimaryDNS", myPrimaryDNS.toString());
      preferences.end();
      navrat += " myPrimaryDNS";
    } else {
      navrat += " !myPrimaryDNS neplatny format!";
    }
  }

  if(doc["mySecondaryDNS"]){
    String ipStr = doc["mySecondaryDNS"].as<String>();
    IPAddress tempIP;
    if (tempIP.fromString(ipStr)) {
      mySecondaryDNS = tempIP;
      preferences.begin("nastavenia", false);
      preferences.putString("mySecondaryDNS", mySecondaryDNS.toString());
      preferences.end();
      navrat += " mySecondaryDNS";
    } else {
      navrat += " !mySecondaryDNS neplatny format!";
    }
  }

  return navrat;
}

/**
   *  poziadavka na ziskanie parametrov z flash pamate
    {  "a":"getflash" }
*/
String getflash(){
  Serial.print("getflash akcia: ");
  String payload = buildJsonConfig();
  return payload; 
}

/**
   *  poziadavka na ziskanie parametrov z flash pamate
    {  "a":"help" }
*/
String send_help(){
  Serial.println("Help akcia:");
  const size_t capacity = 5500;       
  DynamicJsonDocument doc(capacity);
  JsonArray help = doc.createNestedArray("help");

  // manipulacia s relatkami
  { JsonObject c = help.createNestedObject();
    c["action"]="gate"; c["description"]="Zapnutie, lebo predlzenie portu na 5 sek odteraz";
    JsonObject e = c.createNestedObject("example");
      e["a"]="gate"; e["g"]="GATE1"; e["d"]="5";
  }
  { JsonObject c = help.createNestedObject();
    c["action"]="gate"; c["description"]="Zapnutie , alebo predlzenie portu na minimalne min_d";
    JsonObject e = c.createNestedObject("example");
      e["a"]="gate"; e["g"]="GATE2"; e["d"]="5"; e["min_d"]="10";
  }
  { JsonObject c = help.createNestedObject();
    c["action"]="gate"; c["description"]="Okamzite vypnutie portu";
    JsonObject e = c.createNestedObject("example");
    e["a"]="gate"; e["g"]="GATE3"; e["d"]="0";
  }
  { JsonObject c = help.createNestedObject();
    c["action"]="gate"; c["description"]="Zapnutie portu na nekonecne vela";
    JsonObject e = c.createNestedObject("example");
    e["a"]="gate"; e["g"]="GATE4"; e["d"]="11111";
  }

  { JsonObject c = help.createNestedObject();
    c["action"]="gate_ms"; c["description"]="Zapnutie,alebo predlzenie portu na minimalne min_d, MILISEKUNDY";
    JsonObject e = c.createNestedObject("example");
      e["a"]="gate_ms"; e["g"]="GATE1"; e["d"]="3000"; e["min_d"]="10000";
  }
  { JsonObject c = help.createNestedObject();
    c["action"]="gate_ms"; c["description"]="Okamzite vypnutie portu";
    JsonObject e = c.createNestedObject("example");
    e["a"]="gate_ms"; e["g"]="GATE2"; e["d"]="0";
  }
  { JsonObject c = help.createNestedObject();
    c["action"]="gate_ms"; c["description"]="Zapnutie portu na nekonecne vela";
    JsonObject e = c.createNestedObject("example");
    e["a"]="gate_ms"; e["g"]="GATE3"; e["d"]="11111";
  }
  
  // getflash
  { JsonObject c = help.createNestedObject();
    c["action"]="getflash"; c["description"]="Získanie parametrov z flash";
    c.createNestedObject("example")["a"]="getflash";
  }

  // setflash
  { JsonObject c = help.createNestedObject();
    c["action"]="setflash"; c["description"]="Nastavenie parametrov vo flash";
    JsonObject e = c.createNestedObject("example");
    e["a"]="setflash"; e["cas_text1"]=1000;
  }

  // make_update
  { JsonObject c = help.createNestedObject();
    c["action"]="make_update"; c["description"]="OTA update s parametrami";
    JsonObject e = c.createNestedObject("example");
    e["a"]="make_update"; e["ota_host"]="192.168.1.1";
    e["ota_port"]="80"; e["ota_path"]="/firmwares/vaha.bin";
  }

  // ping
  { JsonObject c = help.createNestedObject();
    c["action"]="ping"; c["description"]="Ping hlavnej dosky (zistenie prítomnosti)";
    c.createNestedObject("example")["a"]="ping";
  }

  // reset
  { JsonObject c = help.createNestedObject();
    c["action"]="reset"; c["description"]="Reset celej dosky ESP32";
    c.createNestedObject("example")["a"]="reset";
  }

  // status
  { JsonObject c = help.createNestedObject();
    c["action"]="status"; c["description"]="Informacie o stave zariadenia";
    c.createNestedObject("example")["a"]="status";
  }

  // gate_status
  { JsonObject c = help.createNestedObject();
    c["action"]="gate_status"; c["description"]="Stavy brán GATE1-GATE4 (current_millis, gate_status, gate_off_time)";
    c.createNestedObject("example")["a"]="gate_status";
  }

  // verbose_on
  { JsonObject c = help.createNestedObject();
    c["action"]="verbose_on"; c["description"]="Zapnutie debug výpisov (verbose mode)";
    c.createNestedObject("example")["a"]="verbose_on";
  }

  // verbose_off
  { JsonObject c = help.createNestedObject();
    c["action"]="verbose_off"; c["description"]="Vypnutie debug výpisov (verbose mode)";
    c.createNestedObject("example")["a"]="verbose_off";
  }

  // is_mqtt_allowed
  { JsonObject c = help.createNestedObject();
    c["action"]="mqtt_on"; c["description"]="Zapnutie pouzivania mqtt podla nastavenych hodnot";
    c.createNestedObject("example")["a"]="mqtt_on";
  }

  // is_mqtt_allowed
  { JsonObject c = help.createNestedObject();
    c["action"]="mqtt_off"; c["description"]="Vypnutie pouzivania mqtt podla nastavenych hodnot";
    c.createNestedObject("example")["a"]="mqtt_off";
  }

  if (verbose) {
    Serial.printf("Serializovanie: \n");
  }
  // Konvertovanie JSON do String
  String payload;
  serializeJson(doc, payload);
  return payload;

}



