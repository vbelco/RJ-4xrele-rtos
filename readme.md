# Riadiaca jednotka v1.1 - FreeRTOS verzia

## Obsah
- [Základný popis zariadenia](#základný-popis-zariadenia)
- [FreeRTOS Architektúra](#freertos-architektúra)
- [Inštalácia a oživenie](#inštalácia-oživenie)
- [Používanie](#používanie)
- [Aktualizácia](#aktualizácia)
- [Ďalšie zdroje](#ďalšie-zdroje)

---

## Základný popis zariadenia

Zariadenie pozostáva z nasledujúcich funkčných blokov: základová doska, Lilygo ETH POE (ESP32), 4× relé, servisná LED.

### Komponenty

**Základová doska**  
PCB doska s konektormi pre pripojenie ESP32, 4× relé s vlastnými indikátormi zopnutia (LED), servisná RGB LED zobrazujúca stavy zariadenia a prídavné konektory (GND a 3V3).

**Lilygo ETH POE**  
PCB obsahujúca ESP32, ktorý ovláda komunikáciu a relé. Špecifikácia: https://lilygo.cc/products/t-internet-poe  
Lilygo ETH POE slúži zároveň ako zdroj napätia pre celé zariadenie prostredníctvom POE.

**4× Relé**  
- Typ: SRD-03VDC-SL-C
- Ovládané signálmi úrovne napätia 3V3
- Spínajú napätie do 230V a prúd do 10A
- Každé relé má na základovej doske priradenú LED indikujúcu stav (zopnuté/rozopnuté)
- Výstupné režimy: NO (Normally Open - bez signálu rozopnuté), NC (Normally Closed - bez signálu zopnuté)

**Servisná LED**  
RGB LED ovládaná z ESP32 riadiacim signálom. Zobrazuje nasledujúce stavy zariadenia:

| Farba | LAN | MQTT | Global status | Poznámka |
|-------|-----|------|---------------|----------|
| Blue (modrá) | boot | boot | 0 | Základný stav pri štartovaní zariadenia |
| Red (červená) | NO | NO | 1 | Chybový stav, bez siete |
| Purple (fialová) | Yes | N/A | 2 | MQTT nie je aktivovaný, zariadenie funguje na Serial a API |
| Yellow (žltá) | Yes | NO | 3 | MQTT zapnutý, ale nepripojený |
| Green (zelená) | Yes | Yes | 4 | Všetko v poriadku |

*(N/A - Not Applicable, voľba sa neberie v úvahu)*

---

## FreeRTOS Architektúra

### 🚀 Prehľad

Od verzie **v1.1-rtos** používa zariadenie **FreeRTOS** (Real-Time Operating System) pre efektívnejšie spracovanie úloh na oboch jadrách ESP32.

### 📐 Architektúra systému

```
┌──────────────────────────────────────────────────────────────┐
│                    ESP32 DUAL CORE                           │
│              (FreeRTOS Scheduler Manager)                    │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────┐         ┌────────────────────────┐  │
│  │   INPUT TASKS      │         │   PROCESSING TASK      │  │
│  │  (Priority 1)      │         │   (Priority 2)         │  │
│  │                    │         │                        │  │
│  │  • serialTask      │────┐    │  • relayTask          │  │
│  │  • mqttTask        │────┼───▶│    - Spracovanie      │  │
│  │  • apiTask         │────┘    │      príkazov         │  │
│  │                    │         │    - Kontrola         │  │
│  └────────────────────┘         │      timerov          │  │
│                                 │    - Ovládanie relé   │  │
│  ┌────────────────────┐         └────────────────────────┘  │
│  │  SUPPORT TASKS     │                                     │
│  │  (Priority 1)      │         ┌────────────────────────┐  │
│  │                    │         │   Command Queue        │  │
│  │  • rgbLedTask      │         │   (10 items buffer)    │  │
│  │  • watchdogTask    │         │                        │  │
│  │                    │         │  QueueItem:            │  │
│  └────────────────────┘         │   - jsonCommand        │  │
│                                 │   - source             │  │
│                                 └────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 🔧 FreeRTOS Tasky

Systém je rozdelený do **6 samostatných taskov**:

| Task | Priorita | Stack | Funkcia |
|------|----------|-------|---------|
| **serialTask** | 1 | 4096 | Čítanie Serial portu, odosielanie do queue |
| **mqttTask** | 1 | 8192 | MQTT pripájanie, reconnect, loop handling |
| **apiTask** | 1 | 8192 | HTTP API server handling (`server.handleClient()`) |
| **relayTask** | 2 | 4096 | Spracovanie príkazov z queue, kontrola relé timerov |
| **rgbLedTask** | 1 | 2048 | RGB LED blikanie a stavová indikácia |
| **watchdogTask** | 1 | 2048 | Watchdog reset každých 20 sekúnd |

### 📨 Command Queue

Centrálna **FreeRTOS Queue** pre komunikáciu medzi taskmi:

```cpp
struct QueueItem {
  String jsonCommand;  // JSON príkaz
  String source;       // "serial", "mqtt", "api"
};
```

**Tok dát:**
1. Input tasky (`serialTask`, `mqttTask`, `apiTask`) prijímajú príkazy
2. Vložia ich do `commandQueue`
3. `relayTask` ich vyberá a spracováva
4. Odpoveď sa odošle späť podľa `source`

### ⚙️ Výhody FreeRTOS implementácie

✅ **Paralelné spracovanie** - využitie oboch jadier ESP32  
✅ **Prioritizácia úloh** - relayTask má vyššiu prioritu  
✅ **Žiadne blokujúce `delay()`** - len `vTaskDelay()` ktorý uvoľní CPU  
✅ **Lepšia responsivita** - každý task beží nezávisle  
✅ **Watchdog v samostatnom tasku** - pravidelný reset bez blokovania  
✅ **Modulárny kód** - každý task má svoju funkciu  

### 🗂️ Súborová štruktúra (FreeRTOS vetva)

```
RJ-4xrele-rtos/
├── RJ-4xrele-rtos.ino       # Hlavný súbor - FreeRTOS setup, prázdny loop
├── tasks.ino                # ⭐ Implementácia 6 FreeRTOS taskov
├── mqtt.ino                 # MQTT connect + callback (odosiela do queue)
├── api.ino                  # HTTP API handlery
├── pins_manipulation.ino    # Funkcie zapni/vypni relé
├── rgb_led.ino              # RGB LED funkcie + blikanie
├── utilities.ino            # Pomocné funkcie (get_info, flash, atď.)
├── processJSON.ino          # Spracovanie JSON príkazov (handleJson)
├── ethernet.ino             # Ethernet event handler
├── OTA.ino                  # OTA firmware update
├── utilities.h              # Ethernet PHY definície
└── nastavenia.h             # Globálne premenné a konštanty
```

### 📊 Pamäťová spotreba (FreeRTOS verzia)

```
Program Storage:  1,189,699 / 1,310,720 bytes (90%) ✅
Global Variables:    49,516 /   327,680 bytes (15%) ✅
Free Stack:         278,164 bytes (85%) ✅

FreeRTOS Tasks Stack: ~28 KB total
```

### 🔄 Migrácia z klasickej verzie

**Čo sa zmenilo:**
- ❌ Vymazaný klasický `loop()` - nahradený prázdnym `loop()` s `vTaskDelay()`
- ❌ Vymazaná `processSerial()` funkcia - nahradená `serialTask`
- ✅ Pridaná FreeRTOS queue architektúra
- ✅ Všetky pôvodné funkcie zachované (zapni, vypni, handleJson, atď.)
- ✅ Kompatibilné JSON príkazy
- ✅ Rovnaké MQTT/Serial/API rozhranie

**Čo zostalo:**
- ✅ Všetky JSON príkazy fungujú rovnako
- ✅ MQTT, Serial, API komunikácia identická
- ✅ Ovládanie relé bez zmien
- ✅ RGB LED indikácia zachovaná
- ✅ Flash persistencia, OTA update

---

## Inštalácia, oživenie

Na inštaláciu softvéru a oživenie zariadenia slúži nástroj **binloader** (binloader.ccsipro.sk).

### Kroky inštalácie:
1. Pripojenie USB do Lilygo ETH POE
2. Nahratie programu do ESP32
3. Nastavenie zariadenia
4. Otestovanie funkčnosti zariadenia

### 1. Pripojenie USB do Lilygo ETH POE

Na pripojenie Lilygo ETH POE k počítaču je potrebný programátor (Downloader).

Následne sa použije USB na programátore a pripojí sa k počítaču.

### 2. Nahratie programu do ESP32

1. Otvoriť binloader v prehliadači **Chrome** (iné prehliadače v čase písania článku nepodporujú pripojenie portov)
2. Sekcia "Prototypy" → vybrať prototyp "Riadiaca jednotka" → kliknúť na "Zobraziť na prototyp" → kliknúť na "Nahraj"
3. Otvorí sa okno s monitorom Sériového portu
4. V okne "Konzola nahrávania" kliknúť na **"Pripojiť zariadenie"**
5. Vo vyskakovacom okne vybrať príslušný USB port:
   - Linux: napr. `ttyUSB0`
   - Windows: napr. `COM3`
6. Stlačiť **"Connect"**
7. Prehliadač sa pripojí k ESP32 a v hlavnom okne sa zobrazí komunikácia s ESP32
8. Kliknúť na **"Nahraj"**

💡 **Odporúča sa nedeaktivovať okno prehliadača počas nahrávania.**

### 3. Prvé spustenie (FreeRTOS verzia)

Po nahratí FreeRTOS verzie by ste mali vidieť v Serial Monitore:

```
Wait...
Starting eth:
Command queue created successfully
FreeRTOS tasks created - scheduler running!
Processing to FreeRTOS tasks, good luck!
Watchdog reset
```

To znamená, že FreeRTOS tasky bežia správne! ✅

---

## Používanie

Zariadenie sa používa na základe príkazov v tvare JSON. Príkazy je možné zadávať 3 spôsobmi:

### 1. Serial

Najjednoduchšie je použiť binloader, sekcia Serial:
1. Pripojiť zariadenie na USB
2. Kliknúť na "Pripojiť"
3. Príkazy sa zadávajú v základnom formáte JSON

**Príklad:**
```json
{"a": "ping"}
```

**FreeRTOS verzia:**  
V FreeRTOS verzii `serialTask` číta Serial port a automaticky odosiela príkazy do centrálnej queue.

### 2. MQTT (pokiaľ je povolené)

Vyžaduje aktívne pripojenie k MQTT brokeru, na ktorom je pripojené zariadenie.  
Je nutné vedieť parameter: `nazov_clienta`

Príkazy sú správy obdobne ako v Serial v tvare JSON.

**FreeRTOS verzia:**  
`mqttTask` sa stará o pripájanie, reconnect a prijímanie správ. `mqttCallback` odosiela prijaté príkazy do queue.

### 3. API (GET/POST requesty)

JSON príkazy sa posielajú na známu IP adresu zariadenia buď ako GET alebo POST requesty.

**Formát:**
```
http://192.168.1.X:9090/api?request={"a":"ping"}
```

**FreeRTOS verzia:**  
`apiTask` beží v samostatnom tasku a volá `server.handleClient()` pre obsluhu HTTP requestov.

---

## Základné príkazy

### Ping zariadenia
```json
{"a": "ping"}
```

**Odpoveď:**
```json
{"result": "pingOK"}
```

### Softvérový reštart zariadenia
```json
{"a": "reset"}
```

### Zapnutie relé na určený čas (sekundy)

**Zopnutie relé GATE1 na 10 sekúnd:**
```json
{"a": "gate", "g": "GATE1", "d": 10}
```

Alebo s číslom GPIO pinu:
```json
{"a": "gate", "g": 15, "d": 10}
```

💡 Ak sa príkaz znovu pošle do 10 sekúnd, druhý príkaz **predĺži** trvanie zopnutia o ďalších 10 sec od prijatia príkazu.

**Zopnutie relé GATE1 na nekonečno:**
```json
{"a": "gate", "g": "GATE1", "d": 11111}
```

**Okamžité vypnutie relé GATE1:**
```json
{"a": "gate", "g": "GATE1", "d": 0}
```

### Zapnutie relé na určený čas (milisekundy)

Príkazy sú rovnaké ako v prípade sekundových verzií.

**Zopnutie relé GATE1 na 1000 ms (1 sekunda):**
```json
{"a": "gate_ms", "g": "GATE1", "d": 1000}
```

### Vyžiadanie stavu zariadenia
```json
{"a": "status"}
```

**Odpoveď obsahuje:**
- Hostname, MAC adresa, IP adresa
- UUID, verzia firmware
- Stav všetkých 4 relé
- Časovače relé

### Help ponúkaných príkazov
```json
{"a": "help"}
```

### Výpis aktuálne nastavených parametrov zo zariadenia
```json
{"a": "getflash"}
```

### Nastavenie parametrov zariadenia
```json
{"a": "setflash", "nazov_premennej": "hodnota_premennej"}
```

**Príklad - nastavenie hostname:**
```json
{"a": "setflash", "my_hostname": "rj-garage"}
```

---

## Aktualizácia

Aktualizácia zariadenia na nový firmvér sa realizuje prostredníctvom **binloader** alebo **OTA update**.

### Binloader metóda

1. Otvoriť binloader v prehliadači Chrome
2. Pripojiť zariadenie cez USB
3. Vybrať nový firmvér
4. Kliknúť na "Nahraj"

### OTA (Over-The-Air) Update

Vzdialená aktualizácia cez MQTT alebo API:

```json
{
  "a": "make_update",
  "host": "update.server.com",
  "port": "80",
  "path": "/firmware.bin"
}
```

**FreeRTOS verzia:**  
OTA update beží v `relayTask` s pravidelným watchdog resetom pre dlhé sťahovania.

---

## Ladenie a diagnostika

### Serial Monitor výpis (FreeRTOS)

Pri štarte by ste mali vidieť:

```
Wait...
Load from flash my_hostname => rj-test
Starting eth:
ETH Started
ETH Connected
ETH Got IP
Command queue created successfully
FreeRTOS tasks created - scheduler running!
Processing to FreeRTOS tasks, good luck!
[LED]: LAN:1 MQTT:0 
Watchdog reset
```

### RGB LED diagnostika

Farba LED indikuje stav systému:
- 🔵 **Modrá** - Bootovanie
- 🟢 **Zelená** - LAN OK + MQTT pripojený
- 🟡 **Žltá** - LAN OK + MQTT odpojený
- 🟣 **Fialová** - LAN OK + MQTT vypnutý
- 🔴 **Červená** - Chyba LAN

### Testovanie FreeRTOS taskov

**Test Serial tasku:**
```json
{"a":"ping"}
```
Odpoveď by mala prísť okamžite.

**Test MQTT tasku:**
Publish MQTT správu a sleduj Serial Monitor pre "MQTT message arrived".

**Test API tasku:**
```bash
curl http://192.168.1.X:9090/api?request={"a":"ping"}
```

**Test relayTask:**
```json
{"a":"gate","g":"GATE1","d":"5"}
```
Relé by sa malo zapnúť na 5 sekúnd.

**Test watchdog tasku:**
V Serial Monitore by sa mal každých 20s objaviť:
```
Watchdog reset
```

---

## Ďalšie zdroje

- [Binloader](https://binloader.ccsipro.sk)
- [Dokumentácia API](docs/api.md)
- [Popis hardvérového riešenia](docs/hardware.md)
- [FreeRTOS Dokumentácia](https://www.freertos.org/Documentation/RTOS_book.html)
- [ESP32 FreeRTOS API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)

---

## Changelog

### v1.1-rtos (2026-01-27)
- ✨ Implementácia FreeRTOS architektúry
- ✨ 6 samostatných taskov (serial, mqtt, api, relay, rgb, watchdog)
- ✨ Command queue pre komunikáciu medzi taskmi
- ✨ Využitie oboch jadier ESP32
- ✨ Prioritizácia úloh
- ✨ Žiadne blokujúce delay()
- ✅ Všetky pôvodné funkcie zachované
- ✅ Kompatibilita s existujúcimi JSON príkazmi

### v1.0 (pôvodná verzia)
- ✅ Klasický Arduino loop() štýl
- ✅ MQTT, Serial, API komunikácia
- ✅ 4x relé ovládanie
- ✅ RGB LED indikácia
- ✅ OTA update