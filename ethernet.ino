// Funkcia pre spracovanie Ethernet udalostí
void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname(my_hostname.c_str());
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("ETH Connected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.println(ETH);
      eth_connected = true;
      stav.lan = 1;        // LAN OK
      updateLedFromStav();
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      setRgbColor("red");
      stav.lan = 0;        // LAN DOWN
      updateLedFromStav();
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      stav.lan = 0;        // LAN DOWN
      updateLedFromStav();
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      stav.lan = 0;        // LAN DOWN
      updateLedFromStav();
      break;
    default: break;
  }
}
