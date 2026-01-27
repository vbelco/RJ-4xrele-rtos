# API Dokumentácia - Riadiaca jednotka v1.1

## Prehľad

Riadiaca jednotka podporuje JSON-based API rozhranie prístupné cez:
- **Serial komunikáciu** (USB)
- **HTTP API** (GET/POST requesty)
- **MQTT** (pokiaľ je povolené)

Príkazy sú identické pre všetky tri rozhrania. Pre mqtt a Serial sa JSON príkazy zadávajú priamo. Pre API rozhranie je príkaz "obalený" do formátu requestu - viď nižšie.

---

## HTTP API

### Základný formát

```
http://<IP_ADDRESS>:9090/api?request=<JSON_COMMAND>
```

**Príklad:**
```
http://192.168.1.100:9090/api?request={"a":"ping"}
```

### Metódy

- **GET** - Parameter `request` obsahuje JSON príkaz
- **POST** - JSON príkaz v tele požiadavky (raw)

---

## Príkazy

### 1. Ping

Overenie dostupnosti zariadenia.

**Request:**
```json
{"a": "ping"}
```

**Response:**
```
pingOK
```

**S RID parametrom:**
```json
{"a": "ping", "rid": "abc123"}
```

**Response:**
```json
{"result": "pingOK", "rid": "abc123"}
```

---

### 2. Reset

Softvérový reštart zariadenia.

**Request:**
```json
{"a": "reset"}
```

**Response:**
```
resetSuccess
```

⚠️ **Poznámka:** Zariadenie sa po prijatí príkazu reštartuje.

---

### 3. Gate (ovládanie relé - sekundy)

Zapnutie relé na určený čas v sekundách.

**Request:**
```json
{
  "a": "gate",
  "g": <PIN_NUMBER alebo GATE_NAME>,
  "d": <DURATION_IN_SECONDS>,
  "min_d": <MIN_DURATION> (voliteľné),
  "rid": "request_id" (voliteľné)
}
```

**Parametre:**
- `a`: akcia (vždy "gate")
- `g`: číslo GPIO pinu **alebo** názov gate ("GATE1", "GATE2", "GATE3", "GATE4")
- `d`: doba trvania v sekundách
  - `0` = okamžité vypnutie
  - `1-11110` = čas zopnutia v sekundách
  - `11111` = nekonečno (trvalé zopnutie)
- `min_d`: (voliteľné) minimálna doba trvania - ak je relé vypnuté, použije sa `min_d` namiesto `d`
- `rid`: (voliteľné) request ID pre identifikáciu odpovede

**Príklady:**

Zopnutie relé pomocou čísla GPIO na 10 sekúnd:
```json
{"a": "gate", "g": 15, "d": 10}
```

Zopnutie relé pomocou názvu GATE:
```json
{"a": "gate", "g": "GATE1", "d": 5}
```

Zapnutie s minimálnou dobou (ak je relé vypnuté, zapne sa na 10s, ak je zapnuté, predĺži o 5s):
```json
{"a": "gate", "g": "GATE2", "d": 5, "min_d": 10}
```

Trvalé zopnutie:
```json
{"a": "gate", "g": "GATE3", "d": 11111}
```

Okamžité vypnutie:
```json
{"a": "gate", "g": "GATE4", "d": 0}
```

**Response (bez rid):**

Ak sa relé zaplo (bolo predtým vypnuté):
```
{'action'='relayON','gate'=15}
```

Ak sa relé zaplo natrvalo (d=11111):
```
{'action'='relayONendless','gate'=15}
```

Ak sa relé **manuálne vyplo** (d=0):
```
{'action'='relayOFF','gate'=15}
```

Ak sa len predĺžil čas už zapnutého relé:
```
(prázdny string)
```

**Response (s rid):**
```json
{"result": "{'action'='relayON','gate'=15}", "rid": "abc123"}
```

⚠️ **Poznámka:** Pri **automatickom vypnutí** (po uplynutí času) sa **odpoveď nepošle** - relé sa len vypne a do Serial portu sa vypíše `Vypinam port: X`. Odpoveď `relayOFF` sa posiela **len pri manuálnom vypnutí** príkazom s `d=0`.

⚠️ **Predĺženie času:** Ak sa príkaz znovu pošle pred uplynutím času, trvanie sa predĺži o ďalšiu dobu od momentu prijatia nového príkazu.

---

### 4. Gate MS (ovládanie relé - milisekundy)

Zapnutie relé na určený čas v milisekundách.

**Request:**
```json
{
  "a": "gate_ms",
  "g": <PIN_NUMBER alebo GATE_NAME>,
  "d": <DURATION_IN_MILLISECONDS>,
  "min_d": <MIN_DURATION_MS> (voliteľné),
  "rid": "request_id" (voliteľné)
}
```

**Parametre:**
- `a`: akcia (vždy "gate_ms")
- `g`: číslo GPIO pinu **alebo** názov gate ("GATE1", "GATE2", "GATE3", "GATE4")
- `d`: doba trvania v **milisekundách**
  - `0` = okamžité vypnutie
  - `11111` = nekonečno (trvalé zopnutie)
- `min_d`: (voliteľné) minimálna doba trvania v ms
- `rid`: (voliteľné) request ID pre identifikáciu odpovede

**Príklady:**

Zopnutie relé na 1 sekundu (1000 ms):
```json
{"a": "gate_ms", "g": "GATE1", "d": 1000}
```

Zopnutie s minimálnou dobou:
```json
{"a": "gate_ms", "g": "GATE2", "d": 3000, "min_d": 10000}
```

Trvalé zopnutie:
```json
{"a": "gate_ms", "g": "GATE3", "d": 11111}
```

**Response:**

Ak sa relé zaplo (bolo predtým vypnuté):
```
{'action'='relayON','gate'=15}
```

Ak sa relé zaplo natrvalo (d=11111):
```
{'action'='relayONendless','gate'=15}
```

Ak sa relé **manuálne vyplo** (d=0):
```
{'action'='relayOFF','gate'=15}
```

Ak sa len predĺžil čas už zapnutého relé:
```
(prázdny string)
```

⚠️ **Poznámka:** Pri automatickom vypnutí (po uplynutí času) sa odpoveď nepošle - relé sa len vypne.

---

### 5. Status

Vyžiadanie aktuálneho stavu zariadenia.

**Request:**
```json
{"a": "status"}
```

**Response:**
JSON objekt s aktuálnym stavom zariadenia (výstup funkcie `get_info()`)

**S RID parametrom:**
```json
{"a": "status", "rid": "xyz"}
```

**Response:**
```json
{"result": {...stavove_data...}, "rid": "xyz"}
```

---

### 6. Help

Zobrazenie zoznamu dostupných príkazov.

**Request:**
```json
{"a": "help"}
```

**Response:**
JSON objekt s poľom `help` obsahujúcim všetky dostupné príkazy s popismi a príkladmi.

**Formát:**
```json
{
  "help": [
    {
      "action": "gate",
      "description": "Zapnutie, lebo predlzenie portu na 5 sek odteraz",
      "example": {"a": "gate", "g": "GATE1", "d": "5"}
    },
    ...
  ]
}
```

---

### 7. Get Flash (získanie parametrov)

Výpis aktuálne nastavených parametrov uložených vo flash pamäti.

**Request:**
```json
{"a": "getflash"}
```

**Response:**
JSON objekt s konfiguračnými parametrami (výstup funkcie `buildJsonConfig()`)

**S RID parametrom:**
```json
{"a": "getflash", "rid": "xyz"}
```

**Response:**
```json
{"result": {...config_data...}, "rid": "xyz"}
}
```

---

### 8. Set Flash (nastavenie parametrov)

Nastavenie parametrov zariadenia.

**Request:**
```json
{
  "a": "setflash",
  "nazov_premennej": "hodnota_premennej"
}
```

**Podporované parametre:**
- `my_hostname` - hostname zariadenia (String)
- `broker` - MQTT broker adresa (String)
- `mqtt_port` - MQTT port (unsigned int)
- `mqtt_username` - MQTT užívateľské meno (String)
- `mqtt_password` - MQTT heslo (String)
- `nazov_clienta` - MQTT client ID (String)
- `is_mqtt_allowed` - povolenie MQTT (0 alebo 1)
- `useDHCP` - použiť DHCP (0 alebo 1)
- `myIPAddress` - statická IP adresa (String formát "192.168.1.100")
- `myGateway` - gateway IP (String)
- `mySubnet` - subnet maska (String)
- `myPrimaryDNS` - primárny DNS (String)
- `mySecondaryDNS` - sekundárny DNS (String)

**Príklady:**

Nastavenie MQTT brokera:
```json
{"a": "setflash", "broker": "broker.example.com"}
```

Nastavenie statickej IP:
```json
{"a": "setflash", "useDHCP": 0, "myIPAddress": "192.168.1.100"}
```

Povolenie MQTT:
```json
{"a": "setflash", "is_mqtt_allowed": 1}
```

**Response:**
```
Toto sa nastavilo: broker
```
alebo pri chybe:
```
Toto sa nastavilo: !myIPAddress neplatny format!
```

⚠️ **Poznámka:** Po nastavení niektorých parametrov (napr. sieťové nastavenia) je potrebné vykonať reštart zariadenia príkazom `reset`.

---

### 9. Make Update (OTA aktualizácia)

OTA (Over-The-Air) aktualizácia firmvéru.

**Request:**
```json
{
  "a": "make_update",
  "ota_host": "<SERVER_IP_OR_HOSTNAME>",
  "ota_port": "<PORT>" (voliteľné, predvolené 443),
  "ota_path": "</path/to/firmware.bin>"
}
```

**Parametre:**
- `a`: akcia (vždy "make_update")
- `ota_host`: IP adresa alebo hostname servera s firmvérom (povinné)
- `ota_port`: port servera (voliteľné, predvolené: "443")
- `ota_path`: cesta k .bin súboru firmvéru na serveri (povinné)

**Príklad:**

Aktualizácia z HTTP servera:
```json
{
  "a": "make_update",
  "ota_host": "192.168.1.1",
  "ota_port": "80",
  "ota_path": "/firmwares/rj_v1.2.bin"
}
```

Aktualizácia z HTTPS servera (predvolený port 443):
```json
{
  "a": "make_update",
  "ota_host": "update.example.com",
  "ota_path": "/firmware/latest.bin"
}
```

**Response:**
```
Spracovanie OTA ukoncene
```
alebo pri chybe:
```
Chybajuce, alebo neplatne parametre pre OTA
```

⚠️ **Upozornenie:** 
- Zariadenie sa po úspešnej aktualizácii automaticky reštartuje
- Počas aktualizácie nevypínajte zariadenie
- Port musí byť číslo v rozsahu 1-65535

---

## Spoločné parametre

### RID (Request ID)

Voliteľný parameter `rid` môžete pridať do **akéhokoľvek** príkazu pre jednoduchšiu identifikáciu odpovede.

**Bez RID:**
```json
{"a": "ping"}
```
Odpoveď:
```
pingOK
```

**S RID:**
```json
{"a": "ping", "rid": "abc123"}
```
Odpoveď:
```json
{"result": "pingOK", "rid": "abc123"}
```

---

## MQTT API

### Topic štruktúra

**Publish (odosielanie príkazov):**
```
<CLIENT_NAME>/command
```

```<CLIENT_NAME>``` je identické s premennou "nazov_clienta" a da sa zmenit pomocou setflash príkazu

**Subscribe (príjem odpovede):**
```
res/<CLIENT_NAME>
```
Odpovede zo zariadenia sú odosielané do Topicu s predponou res/...

### Príklad použitia

**Odoslanie príkazu:**
- Topic: `/prototip/test_riadiaca_jednotka`
- Payload: `{"a":"gate","g":"GATE1","d":10}`

**Odpoveď:**
- Topic: `res-prototip/test_riadiaca_jednotka`
- Payload: `{'action'='relayON','gate'=15}`

---

## Serial API

### Komunikačné parametre

- **Baudrate:** 115200
- **Data bits:** 8
- **Parity:** None
- **Stop bits:** 1

### Použitie

Príkazy sa zadávajú ako JSON reťazce ukončené znakom nového riadku (`\n`).

**Príklad:**
```
{"a":"ping"}\n
```

---

## Chybové stavy

### Formát chybovej odpovede

Chybové odpovede sú vo forme jednoduchých textových reťazcov:

```
error: missing action parameter
error: empty action parameter  
action gate failed
Invalid GPIO pin
unknown request
Chybajuce, alebo neplatne parametre pre OTA
```

**S RID parametrom:**
```json
{"result": "unknown request", "rid": "abc123"}
```

### Bežné chyby

| Chyba | Význam |
|-------|--------|
| `error: missing action parameter` | Chýba povinný parameter `a` v JSON |
| `error: empty action parameter` | Parameter `a` je prázdny |
| `action gate failed` | Chýbajú povinné parametre `g` alebo `d` |
| `Invalid GPIO pin` | Neplatný GPIO pin (nie je GATE1-4) |
| `unknown request` | Neznámy príkaz |
| `Chybajuce, alebo neplatne parametre pre OTA` | Chýbajú `ota_host` alebo `ota_path` |

