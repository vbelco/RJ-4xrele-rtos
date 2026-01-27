# Riadiaca jednotka v1.1 - verzia Ethernet

## Obsah
- [Základný popis zariadenia](#základný-popis-zariadenia)
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

### 2. MQTT (pokiaľ je povolené)

Vyžaduje aktívne pripojenie k MQTT brokeru, na ktorom je pripojené zariadenie.  
Je nutné vedieť parameter: `nazov_clienta`

Príkazy sú správy obdobne ako v Serial v tvare JSON.

### 3. API (GET/POST requesty)

JSON príkazy sa posielajú na známu IP adresu zariadenia buď ako GET alebo POST requesty.

**Formát:**
```
http://127.0.0.1:9090/api?request={"a":"ping"}
```

---

## Základné príkazy

### Ping zariadenia
```json
{"a": "ping"}
```

### Softvérový reštart zariadenia
```json
{"a": "reset"}
```

### Zapnutie relé na určený čas (sekundy)

**Zopnutie relé na pine 15 na 10 sekúnd:**
```json
{"a": "gate", "g": 15, "d": 10}
```
💡 Ak sa príkaz znovu pošle do 10 sekúnd, druhý príkaz **predĺži** trvanie zopnutia o ďalších 10 sec od prijatia príkazu.

**Zopnutie relé na pine 15 na nekonečno:**
```json
{"a": "gate", "g": 15, "d": 11111}
```

**Okamžité vypnutie relé na pine 15:**
```json
{"a": "gate", "g": 15, "d": 0}
```

### Zapnutie relé na určený čas (milisekundy)

Príkazy sú rovnaké ako v prípade sekundových verzií.

**Zopnutie relé na pine 15 na 1000 ms (1 sekunda):**
```json
{"a": "gate_ms", "g": 15, "d": 1000}
```

### Vyžiadanie stavu zariadenia
```json
{"a": "status"}
```

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

---

## Aktualizácia

Aktualizácia zariadenia na nový firmvér sa realizuje prostredníctvom **binloader**.

1. Otvoriť binloader v prehliadači Chrome
2. Pripojiť zariadenie cez USB
3. Vybrať nový firmvér
4. Kliknúť na "Nahraj"

---


## Ďalšie zdroje

- [Binloader](https://binloader.ccsipro.sk)
- [Dokumentácia API](docs/api.md)
- [Popis hardvérového riešenia](docs/hardware.md)

