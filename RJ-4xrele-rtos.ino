/**
*               Riadiaca  Jednotka - FreeRTOS Version
*               CCSIPRO™
**/
#include <map>
#include <Preferences.h> // esp32 corekniznica na pracu s flash pamatou
#include "utilities.h" // eth doska nastavenie pinov
#include <ETH.h> //esp32 core
#include <PubSubClient.h> 
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <Update.h>
#include "nastavenia.h"
#include <esp_task_wdt.h>
#include <Wire.h>
#include <Adafruit_PN532.h> // https://github.com/adafruit/Adafruit-PN532
#include <WebServer.h>      //esp32 core

// FreeRTOS includes
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

uint64_t IRAM_ATTR myMillis() {
  return (uint64_t)(esp_timer_get_time() / 1000);  // Mikrosekundy na milisekundy
}
#define millis myMillis

#define MQTT_MAX_PACKET_SIZE 1024  // nastavenie vlastneho limitu pre MQTT spravu
const int SAFE_LENGTH = 100; //bezpecna dlzka odpovede na mqtt vsetko kratsie pojde cez mqtt.publish(), vsetko dlhsie pojde cez beginPublish() + write() + endPublish()

//forward deklacie
void initialize_rgb_led();
void setRgbBrightness(uint8_t brightness);
void turnOffRgb();
void setRgbColor(uint8_t r, uint8_t g, uint8_t b);
void setRgbColor(String color, unsigned int frequency = 0, String mode = "flash");
void processRgbBlink();
void updateLedFromStav();
String handleJson(DynamicJsonDocument& doc, const String& request_from = "none");

//premenna obsahujuca globalny stav led podla readme.md
struct Stav {
  uint8_t lan;      // 0 = NO, 1 = YES, 2 = BOOT
  uint8_t mqtt;     // 0 = NO, 1 = YES, 2 = N/A
};

Stav stav = {
  .lan = 2,
  .mqtt = 2,
};

// FreeRTOS Queue pre príkazy
#define QUEUE_JSON_SIZE 2048  // 2KB pre väčšie JSON príkazy s parametrami
#define QUEUE_SOURCE_SIZE 16

struct QueueItem {
  char jsonCommand[QUEUE_JSON_SIZE];
  char source[QUEUE_SOURCE_SIZE]; // "serial", "mqtt", "api"
};

QueueHandle_t commandQueue;

// Layers stack
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

//Webserver API
WebServer server(WEB_SERVER_PORT); // Nastavenie webového servera
long timeout_kontroly_api = 5000; // 5000 = 5sec -> cas, pokial budeme cakat na navrat zo servera api,

unsigned long currentMillis;       // uchovavanie aktualneho casu v loope programu
uint32_t lastReconnectAttempt = 0; // uchovanie posledneho pripojenia, pre non blocking pripajanie
/* priznak posledneho pokusu o pripojenie k brokeru
   0 posledne bolo pripojenie, alebo vypnute
   1, 2, 3 pocet poslednych neuspesnych pripojeni, po 3 nasleduje reset esp
*/
unsigned int last_connection_attempt = 0;

unsigned long dlzka_reportovania = 0; /* dlzka odosielania reportov o stave zariadenia, bude sa skracovat ako bude plynut cas, az do 0 */
unsigned long frekvencia_reportovania = 0; /* frekvencia odosielania reportov o stave zariadenia */
unsigned long last_report_millis = 0; /* timestamp posledneho reportovania, je v millis */
unsigned int report_active = 0; /* priznak o aktivnom reportovani */
unsigned long last_dtw_millis = 0; /* pomocna premenna na watchdog */
//stavy NEKONECNEHO otvorenia a zatvorenia brany, false = gateDOWN a true = GATEUP
bool gate1_status = false;
bool gate2_status = false;
bool gate3_status = false;
bool gate4_status = false;

Preferences preferences; /* spristupnenie flash pamate */

//minimalna dlzka otvorenia brany
//po prichode prveho prikazu bude brana minimalne tento cas otvorenia
unsigned int minimal_gate_open_time = 0; //sec


/**
 *  S E T U P
 */

void setup()
{
  // inicializacia pinov
  initialize_pins();

  // Inicializácia RGB LED
  initialize_rgb_led();

  // Set console baud rate
  Serial.begin(115200);

  delay(500);
  Serial.println("Wait...");

  proceess_preferences();//nastavenie flash pamate

  say_hello(); //uvodna hlaska o vlastnej identifikacii

  //Startovanie ETH
  Serial.println("Starting eth:");
  Network.onEvent(onEvent); // Registrovanie Ethernet udalostí

  pinMode(NRST, OUTPUT);

  digitalWrite(NRST, 0);  delay(200);
  digitalWrite(NRST, 1);  delay(200);
  digitalWrite(NRST, 0);  delay(200);
  digitalWrite(NRST, 1);  delay(150);

  if (useDHCP) {
    Serial.println("Pouzivam plne DHCP: ");
  } else {
    Serial.println("Pouzivam manualne nastavenia: ");
    // Ak niektoré hodnoty sú 0.0.0.0, použije sa DHCP len pre ne
    IPAddress config_IP = (myIPAddress == IPAddress(0, 0, 0, 0)) ? INADDR_NONE : myIPAddress;
    IPAddress config_Gateway = (myGateway == IPAddress(0, 0, 0, 0)) ? INADDR_NONE : myGateway;
    IPAddress config_Subnet = (mySubnet == IPAddress(0, 0, 0, 0)) ? INADDR_NONE : mySubnet;
    IPAddress config_PrimaryDNS = (myPrimaryDNS == IPAddress(0, 0, 0, 0)) ? INADDR_NONE : myPrimaryDNS;
    IPAddress config_SecondaryDNS = (mySecondaryDNS == IPAddress(0, 0, 0, 0)) ? INADDR_NONE : mySecondaryDNS;

    // Nastavenie statickej IP pre Ethernet
    if (!ETH.config(config_IP, config_Gateway, config_Subnet, config_PrimaryDNS, config_SecondaryDNS)) {
      Serial.println("Chyba pri nastavení statickej IP adresy pre Ethernet!");
      return;
    }
  }

  delay(150);
  if (!ETH.begin()) {
    Serial.println("Nepodarilo sa nastartovat ETH");
  } else {
    Serial.println("ETH started");
  }

  //cakanie na priradenie IP adresy - 10 sec
  unsigned long startWait = millis();
  while (!eth_connected && millis() - startWait < 10000) {
    delay(100);
    Serial.print(".");
  }
  if (!eth_connected) {
    Serial.println("\n Nemam IP adresu do 10 sec, RESTARTUJ MANUALNE!!!");
  }
  Serial.println("\n Ethernet pripojeny");

  // MQTT Broker setup
  espClient.setInsecure(); //volba nepouzit certifikat
  mqtt.setServer(broker.c_str(), mqtt_port);
  mqtt.setCallback(mqttCallback);

  //nacitanie stavov brany 
  preferences.begin("gate_status");
  gate1_status = preferences.getBool("gate1_status", gate1_status);
  gate2_status = preferences.getBool("gate2_status", gate2_status);
  gate3_status = preferences.getBool("gate3_status", gate3_status);
  gate4_status = preferences.getBool("gate4_status", gate4_status);
  preferences.end();

  //nekonecne pozapinanie relatok na zaklade flash
  if (gate1_status == true) zapni_endless(GATE1);
  if (gate2_status == true) zapni_endless(GATE2);
  if (gate3_status == true) zapni_endless(GATE3);
  if (gate4_status == true) zapni_endless(GATE4);

  //spracovanie prichadzajucej poziadavky
  server.on("/api", HTTP_GET, apiHandleGetRequest);
  server.on("/api", HTTP_POST, apiHandlePostRequest);
  server.on("/setParams", HTTP_GET, setParams); //nastavi parameter z nastavenia.h
  server.on("/getParams", HTTP_GET, getParams); // ziska vsetky parametre z nastavenia.h

  server.begin();

  /* Nastavenie watchdogu */
  Serial.println("Configuring WDT...");
  esp_task_wdt_config_t twdt_config = {
    .timeout_ms = WDT_TIMEOUT,
    .idle_core_mask = (1 << 0) | (1 << 1),    // nastavenie jadra 0 aj jadra 1 na monitoring WDT
    .trigger_panic = true, //zapnutie panic vypisu v pripade nastatia WDT
  };
  esp_task_wdt_deinit();// Deinicializujeme TWDT, ak je už inicializovaný, aby sme ho mohli nakonfigurovať nanovo
  ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
  ESP_ERROR_CHECK(esp_task_wdt_add(NULL)); // pridanie Arduino loop task (aktuálny task)
  printf("TWDT initialized\n");
  last_dtw_millis = millis(); //pociatocne nastavenie casovacia na wdt

  // ========================================
  // FreeRTOS SETUP
  // ========================================
  
  // Vytvorenie FreeRTOS command queue
  commandQueue = xQueueCreate(10, sizeof(QueueItem));
  if (commandQueue == NULL) {
    Serial.println("Failed to create command queue!");
  } else {
    Serial.println("Command queue created successfully");
  }
  
  // Vytvorenie FreeRTOS taskov (scheduler rozhoduje o core)
  // Stack sizes zvýšené kvôli veľkým JSON dokumentom (send_help = 5.5KB)
  xTaskCreate(serialTask,    "Serial",    6144, NULL, 1, NULL);   // 6KB - QueueItem + obmedzený buffer
  xTaskCreate(mqttTask,      "MQTT",      8192, NULL, 1, NULL);   // 8KB
  xTaskCreate(apiTask,       "API",      20480, NULL, 1, NULL);   // 20KB - String + JsonDoc + send_help
  xTaskCreate(relayTask,     "Relay",    20480, NULL, 2, NULL);   // 20KB - QueueItem + JsonDoc + send_help
  xTaskCreate(rgbLedTask,    "RGB",       2048, NULL, 1, NULL);   // 2KB
  
  Serial.println("FreeRTOS tasks created - scheduler running!");
  Serial.println("Processing to FreeRTOS tasks, good luck!");
}

/*********************************************
*  L O O P  (prázdny - FreeRTOS prevezme kontrolu)
**********************************************/
void loop()
{
  // Reset watchdogu pre loopTask (každých 15s, timeout je 40s)
  esp_task_wdt_reset();
  
  // Prázdny loop - FreeRTOS scheduler prevezme kontrolu
  vTaskDelay(pdMS_TO_TICKS(15000)); // 15 sekúnd
} // end loop
