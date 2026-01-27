# Hardware dokumentácia - Riadiaca jednotka v1.1

## Prehľad

Riadiaca jednotka v1.1 je zariadenie určené na ovládanie 4 relé prostredníctvom ESP32 s Ethernet POE pripojením.

---

## Hardvérové komponenty

### 1. Lilygo ETH POE (T-Internet-POE)

**Základné špecifikácie:**
- **MCU:** ESP32-WROOM-32
- **Flash:** 4MB
- **PSRAM:** Nie
- **Ethernet:** 10/100 Mbps s POE (Power over Ethernet)
- **POE štandard:** IEEE 802.3af (max 15.4W)
- **Napájanie:**
  - POE: 37-57V DC vstup
  - Výstup: 5V/2.4A
- **GPIO piny:** Prístupné cez konektory
- **USB:** Type-C pre programovanie

**Užitočné linky:**
- [Oficiálna stránka produktu](https://lilygo.cc/products/t-internet-poe)
- [Schéma zapojenia](https://github.com/Xinyuan-LilyGO/LilyGO-T-ETH-POE)
- [Pinout diagram](https://github.com/Xinyuan-LilyGO/LilyGO-T-ETH-POE/blob/master/doc/pins.md)

---

### 2. Základová doska (Custom PCB)

**Funkcie:**
- Mechanické uchytenie Lilygo ETH POE
- Konektory pre 4× relé moduly
- Servisná RGB LED
- Indikačné LED pre každé relé
- Rozvodné konektory (GND, 3V3)

**Rozmer:** TBD

**Materiál:** FR-4, 1.6mm

---

### 3. Relé moduly

**Špecifikácia:**
- **Typ:** SRD-03VDC-SL-C (1-kanálové relé)
- **Počet:** 4 ks
- **Riadiace napätie:** 3.3V DC
- **Riadiaci prúd:** ~70mA
- **Spínacie kontakty:**
  - COM (Common)
  - NO (Normally Open)
  - NC (Normally Closed)
- **Maximálne spínacie napätie:** 250V AC / 30V DC
- **Maximálny spínaný prúd:** 10A
- **Životnosť:** 100,000 cyklov (elektr.), 10,000,000 cyklov (mech.)

**Indikácia:**
- LED na relé module: indikuje aktiváciu cievky
- LED na základovej doske: duplicitná indikácia pre každé relé

---

### 4. Servisná RGB LED

**Typ:** WS2812B / SK6812 (adresovateľná RGB LED)

**Pripojenie:**
- Dátový pin: GPIO (definovaný v `rgb_led.ino`)
- Napájanie: 3.3V/5V
- Počet LED: 1

**Funkcia:** Zobrazenie stavu zariadenia (LAN, MQTT, celkový stav)

---

## Pinout ESP32

### Použité GPIO piny

| GPIO | Funkcia | Popis |
|------|---------|-------|
| GPIO 4 | RELÉ 1 | Ovládanie prvého relé |
| GPIO 12 | RELÉ 2 | Ovládanie druhého relé |
| GPIO 15 | RELÉ 3 | Ovládanie tretieho relé |
| GPIO 33 | RELÉ 4 | Ovládanie štvrtého relé |
| GPIO 32 | RGB LED | Servisná LED (Data pin) |

---

## Elektrické charakteristiky

### Napájanie

**POE vstup:**
- Napätie: 37-57V DC (IEEE 802.3af)
- Maximálny príkon: 15.4W
- Odhadovaná spotreba:
  - ESP32: ~500mA @ 3.3V (1.65W)
  - 4× relé (všetky aktívne): 4 × 70mA @ 3.3V (0.92W)
  - RGB LED: ~60mA @ 3.3V (0.2W)
  - **Celková spotreba:** ~2.8W (pri plnom zaťažení)

**Prídavné konektory:**
- 3.3V výstup: Max 500mA (pre externé obvody)
- GND: spoločná zem


## Ochrana a bezpečnosť

### Elektrická bezpečnosť

⚠️ **VAROVANIE:** Relé spínajú sieťové napätie 230V AC. Vždy dodržujte bezpečnostné predpisy!

**Odporúčania:**
- Inštaláciu vykonávať iba kvalifikovaná osoba
- Používať vodič s dostatočným prierezom (min. 1.5mm² pre 10A)
- Zariadenie umiestniť do ochrannej skrinky (IP20 min.)
- Zabrániť prístupu neoprávnených osôb k vodičom pod napätím


## Programovanie


### Programovanie cez USB

1. Pripojte programátor k Lilygo
2. Pripojte USB kábel k programátoru a PC
3. Zariadenie sa zobrazí ako sériový port:
   - Linux: `/dev/ttyUSB0` (alebo podobne)
   - Windows: `COMx`
4. Použite binloader alebo Arduino IDE na nahratie firmvéru

## Údržba

### Pravidelná kontrola

- Kontrola pevnosti pripojení vodičov (každých 6 mesiacov)
- Vizuálna kontrola stavu relé (známky opálenia)
- Kontrola funkčnosti indikačných LED
- Meranie teploty pri prevádzke (nemala by presiahnuť 60°C)


---

## Prílohy

- [Schéma základovej dosky (PDF)](images/schematic.pdf)
- [PCB layout (Gerber files)](files/pcb_gerber.zip)
- [Datasheet relé SRD-03VDC-SL-C](datasheets/relay_datasheet.pdf)
- [Datasheet Lilygo ETH POE](datasheets/lilygo_eth_poe.pdf)
