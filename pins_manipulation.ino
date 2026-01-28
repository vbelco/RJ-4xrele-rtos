/**
 * inicializacia GPIO
 */
void initialize_pins()
{
  for (const auto& [pin, _] : koniec) { //prebehne definiciu map a podla nej nastavi piny
    pinMode(pin, OUTPUT);
    digitalWrite(pin, GATE_DOWN);
    pinStav[pin] = GATE_DOWN; // inicializacia logickeho stavu
  }
}

/**
 * fcia na zapnutie portu
 */
String zapni(unsigned int port, unsigned int trvanie, bool milisekundy = false)
{
  String odpoved = "";
  unsigned long ul_trvanie = (unsigned long)trvanie; // nutna konverzia vzhladom na fciu millis
  if (!milisekundy) ul_trvanie *= 1000; // ked niesu definovane milisekundy, tak skonvertne na milisekundy
  // TODO
  // loogika posuvania brany vzhladom na minimalnu dlzku otvorenia
  long novyCas = (long)(currentMillis + ul_trvanie); // explicitna konverzia na long
  if (koniec[port] < novyCas){ //ak uz prekracujeme maximminimalnu hranicu otvorenia brany
    koniec[port] = novyCas;         // nastavenie konca
  }

  /* ak je brana vypnuta a zapli sme ju, tak odpoviwm, ze sme ju zapli*/
  if (pinStav[port] == GATE_DOWN){ /*ak bola predtym brana dole, dame oznam o jej dvihani*/
    //nastavenie minimalneho konca otvorenia brany
    String str_port = String(port);
    odpoved = "{'action'='relayON','gate'="+str_port+"}";
  }

  digitalWrite(port, GATE_UP); /*zdvihneme branu*/ 
  pinStav[port] = GATE_UP; // aktualizacia logickeho stavu
  return odpoved;
}

/**
*  fcia na nekonecne zapnutie portu
**/
String zapni_endless(unsigned int port){
  koniec[port] = 0; //nastavim si koniec vypnutia na 0, teda pre nas nekonecno
  digitalWrite(port , GATE_UP);  // zapnem port
  pinStav[port] = GATE_UP; // aktualizacia logickeho stavu
  String str_port = String(port);
  String odpoved = "{'action'='relayONendless','gate'="+str_port+"}";
  return odpoved;
}


/**
 * fcia na vypnutie portu
 */
String vypni(unsigned int port)
{
  String odpoved = "";
  /* ak je brana zapnuta a vypiname ju, tak odpoviwm, ze sme ju vypli*/
  if (pinStav[port] == GATE_UP){ /*ak bola predtym brana hore, dame oznam o jej vypinani*/
    String str_port = String(port);
    String odpoved = "{'action'='relayOFF','gate'="+str_port+"}";
    Serial.printf("Vypinam port: %d \n", port);
  }
  digitalWrite(port , GATE_DOWN); /*klesnutie brany*/
  pinStav[port] = GATE_DOWN; // aktualizacia logickeho stavu
  koniec[port] = -1; // nastavenie portu ako vypnuty
  return odpoved;
}
