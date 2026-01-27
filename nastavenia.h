/**
*               Riadiaca jednotka
*               CCSIPRO™
**/
#ifndef nastavenia_h
#define nastavenia_h

// MQTT details
String broker = "dev4.ccsipro.sk";
unsigned int mqtt_port = 8883; /* port na pripajanie */
String mqtt_username = "prototip";
String mqtt_password = "Rff5Gggs899";
unsigned int is_mqtt_allowed = 0; //priznak, ci je realizovane pripojenie cez mqtt

//zakladna vlastna identifikacia
String my_hostname = "eth-lilygo";
String version   = "4.0.0"; //verzia jadra 3.3.5
String uuid = ""; //ak je prazdna vygeneruje sa a ulozi sa do Flash, tato sa neda nastavit cez preferences ani tam nieje ulozena!

//Ak IP adresa je myIPAddress(0, 0, 0, 0) bude nahradena pridelenou zo siete inac ip adresa bude zadana statisky. Ostatne parametre budu z dhcp 
bool useDHCP = true;  // true = DHCP, false = manuálne nastavenie

IPAddress myIPAddress(10, 16, 41, 100);   
IPAddress myGateway(10, 16, 41, 1);       // Brána (zvyčajne router)
IPAddress mySubnet(255, 255, 255, 0);    // Maska podsiete
IPAddress myPrimaryDNS(8, 8, 8, 8);      // Primárny DNS (napr. Google DNS)
IPAddress mySecondaryDNS(1, 1, 1, 1);    // Sekundárny DNS (voliteľné)

String nazov_clienta = "system/vbelco/eth-liligo2"; /* identifikacia clienta do sluzby mqtt  */
String nazov_prijimacieho_kanala = nazov_clienta; /*nazov kanala na ktorom budeme pocuvat*/
String nazov_odosielacieho_kanala = "res/"+nazov_clienta; //*nazov kanala do ktoreho odpovedame*/
String nazov_lwt = "lwt"; /*nazov kanala kde sa posiela last will sprava*/
String nazov_online = "online"; /*nazov kanala kde sazariadenie prihlasi ked sa pripoji na broker*/

//40 seconds WDT
#define WDT_TIMEOUT 40000 //hodnota je v ms

//nastavenie web servera na ktorom bude pocuvat esp32
#define WEB_SERVER_PORT 9090 //port na ktorom pocuva esp32 api server

/*nastavenie stavu relatok, cije HIGH zopnute alebo rozopnute*/
#define GATE_UP HIGH
#define GATE_DOWN LOW

/* definicie pinov na branach MUSIA BYT definovane 4, MOZU smerovat na neexistujuci port napr #define GATE4 99*/
#define GATE1 4 //rele 
#define GATE2 12 //rele 
#define GATE3 15 //rele 
#define GATE4 33 //rele 

/* definicia mapu konca casov vypnutia portov  -> musi zodpovedat definicii pinov
 * -1 = port je vypnuty (GATE_DOWN)
 *  0 = port je zapnuty natrvalo (nekonecne)
 * >0 = port sa vypne v case koniec[port]
 */
std::map<int, long> koniec{
    {GATE1, -1},
    {GATE2, -1},
    {GATE3, -1},
    {GATE4, -1}
};
std::map<int, long>::iterator itr; // definicia iteratora pre map konca casov

/** nastavenie RGB led */
#define RGB_LED_PIN 32 // na ktorom pine je zavesena
#define RGB_LED_COUNT 1 //pocet lediek

#endif
