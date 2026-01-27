/**
*               RGB LED WS2812
*               CCSIPRO™
**/

#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel_ZeroDMA

Adafruit_NeoPixel rgbLed(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// Premenné pre blikanie LED
unsigned long lastBlinkMillis = 0;
bool ledBlinkState = false;
String currentBlinkMode = ""; // "flash" alebo "pulse"
unsigned int currentBlinkFreq = 0;
uint8_t currentR = 0, currentG = 0, currentB = 0;
int pulseCount = 0;

/**
 * Inicializácia RGB LED
 */
void initialize_rgb_led() {
  rgbLed.begin();
  rgbLed.setBrightness(5); // Nastavenie jasu (0-255)
  rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255)); // Modrá farba pri štarte
  rgbLed.show();
}


/**
 * Nastavenie jasu RGB LED
 * @param brightness - jas (0-255)
 */
void setRgbBrightness(uint8_t brightness) {
  rgbLed.setBrightness(brightness);
  rgbLed.show();
}

/**
 * Vypnutie RGB LED
 */
void turnOffRgb() {
  rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 0));
  rgbLed.show();
}

/**
 * Nastavenie farby RGB LED
 * @param r - červená (0-255)
 * @param g - zelená (0-255)
 * @param b - modrá (0-255)
 */
void setRgbColor(uint8_t r, uint8_t g, uint8_t b) {
  rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
  rgbLed.show();
}


/**
 * Wrapper pre nastavenie farby LED s blikáním
 * @param color - názov farby: "blue", "red", "yellow", "green"
 * @param frequency - frekvencia blikania v ms (0 = stále svieti, default)
 * @param mode - režim blikania: "flash" (nekonečné, default) alebo "pulse" (1x bliknutie)
 */
/*
 * Pouzitie:
 * setRgbColor("blue");              // Modrá stále svieti
 * setRgbColor("red", 500);          // Červená bliká každých 500ms donekonečna
 * setRgbColor("yellow", 300, "pulse"); // Žltá blikne 1x po 300ms a potom svieti
 * setRgbColor("green", 1000, "flash"); // Zelená bliká každú sekundu
*/
void setRgbColor(String color, unsigned int frequency, String mode) {
  // Resetovanie blikania
  currentBlinkMode = mode;
  currentBlinkFreq = frequency;
  pulseCount = 0;
  ledBlinkState = false;
  lastBlinkMillis = millis();
  
  // Nastavenie farby podľa názvu
  if (color == "blue") {
    currentR = 0; currentG = 0; currentB = 255;
  } else if (color == "red") {
    currentR = 255; currentG = 0; currentB = 0;
  } else if (color == "yellow") {
    currentR = 255; currentG = 255; currentB = 0;
  } else if (color == "green") {
    currentR = 0; currentG = 255; currentB = 0;
  } else if (color == "cyan") {
    currentR = 0; currentG = 255; currentB = 255;
  } else if (color == "purple") {
    currentR = 128; currentG = 0; currentB = 128;
  } else if (color == "orange") {
    currentR = 255; currentG = 165; currentB = 0;
  } else {
    // Neznáma farba - vypnúť LED
    currentR = 0; currentG = 0; currentB = 0;
  }
  
  // Ak je frekvencia 0 alebo režim prázdny, zobraziť farbu natrvalo
  if (frequency == 0 || mode == "") {
    currentBlinkMode = "";
    currentBlinkFreq = 0;
    setRgbColor(currentR, currentG, currentB);
  } else {
    // Zapnúť LED na začiatku
    setRgbColor(currentR, currentG, currentB);
    ledBlinkState = true;
  }
}

/**
 * Funkcia na spracovanie blikania LED - musí sa volať v loop()
 */
void processRgbBlink() {
  // Ak nie je nastavené blikanie, nič nerobíme
  if (currentBlinkFreq == 0 || currentBlinkMode == "") {
    return;
  }
  
  unsigned long currentMillis = millis();
  
  // Kontrola, či uplynul čas na prepnutie stavu
  if (currentMillis - lastBlinkMillis >= currentBlinkFreq) {
    lastBlinkMillis = currentMillis;
    
    // Prepnutie stavu LED
    ledBlinkState = !ledBlinkState;
    
    if (ledBlinkState) {
      setRgbColor(currentR, currentG, currentB);
    } else {
      setRgbColor(0, 0, 0); // Vypnúť
      
      // Pre pulse režim počítame bliknutia
      if (currentBlinkMode == "pulse") {
        pulseCount++;
        if (pulseCount >= 1) {
          // Po 1 bliknutí zastaviť
          currentBlinkMode = "";
          currentBlinkFreq = 0;
          //setRgbColor(currentR, currentG, currentB); // Zostať svietiť
          updateLedFromStav(); // nastavit povodne hodnoty
        }
      }
    }
  }
}

/**
 * @brief Funkcia na vysvietenie LED podla stavu zariuadenia
 * @return  void
 */
void updateLedFromStav() {
  Serial.print("[LED]:");
  printStavLed();

  switch ((stav.lan << 4) | stav.mqtt) {
    case 0x22: setRgbColor("blue");   break; // Boot (lan=2, mqtt=2)
    case 0x00: case 0x01: case 0x02:         // No LAN (lan=0, any mqtt)
      setRgbColor("red"); break;
    case 0x12: setRgbColor("purple"); break; // LAN OK, MQTT disabled
    case 0x10: setRgbColor("yellow"); break; // LAN OK, MQTT disconnected
    case 0x11: setRgbColor("green");  break; // LAN OK, MQTT connected
    default:   setRgbColor("red");    break; // Fallback
  }
}

/**
 * @brief Preťažená funkcia - nastaví LAN stav a aktualizuje LED
 * @param lan_stav - nový stav LAN (0=NO, 1=YES, 2=BOOT)
 * @return void
 */
void updateLedFromStav(uint8_t lan_stav) {
  stav.lan = lan_stav;
  updateLedFromStav();
}

/**
 * @brief Preťažená funkcia - nastaví LAN a MQTT stav a aktualizuje LED
 * @param lan_stav - nový stav LAN (0=NO, 1=YES, 2=BOOT)
 * @param mqtt_stav - nový stav MQTT (0=NO, 1=YES, 2=N/A)
 * @return void
 */
void updateLedFromStav(uint8_t lan_stav, uint8_t mqtt_stav) {
  stav.lan = lan_stav;
  stav.mqtt = mqtt_stav;
  updateLedFromStav();
}


/**
 * @brief Vypíše stav LED do Serial monitora
 * @return void
 */
void printStavLed() {
  Serial.printf("LAN:%d MQTT:%d \n", stav.lan, stav.mqtt );
}


