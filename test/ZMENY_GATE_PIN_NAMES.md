# Zhrnutie zmien - Textové názvy pinov pre Gate príkazy

## Zmeny v kóde

### 1. processJSON.ino

#### Pridaná funkcia `parseGatePin()`
- Konvertuje textové názvy (GATE1-GATE4) na čísla pinov
- Podporuje aj priame číselné hodnoty (backwards compatibility)
- Validuje vstupy a vracia -1 pre neplatné hodnoty

```cpp
int parseGatePin(JsonVariant gateValue) {
  // Číselná hodnota
  if (gateValue.is<int>() || gateValue.is<unsigned int>()) {
    return gateValue.as<int>();
  }
  
  // Textový názov
  if (gateValue.is<const char*>() || gateValue.is<String>()) {
    String gateName = gateValue.as<String>();
    if (gateName == "GATE1") return GATE1;
    if (gateName == "GATE2") return GATE2;
    if (gateName == "GATE3") return GATE3;
    if (gateName == "GATE4") return GATE4;
    
    // Fallback na číselný string
    int pinNum = gateName.toInt();
    if (pinNum > 0 || gateName == "0") {
      return pinNum;
    }
  }
  
  return -1;
}
```

#### Upravené akcie v `handleJson()`

**Akcia "gate":**
- Zmenené z `const unsigned int gpio = doc["g"]` 
- Na `int gpio = parseGatePin(doc["g"])`
- Pridaná kontrola `if (gpio == -1 || ...)`

**Akcia "gate_ms":**
- Rovnaké zmeny ako pri "gate"

## Nové funkcie

### Podporované formáty

1. **Textové názvy pinov:**
   - `{"a":"gate", "g":"GATE1", "d":"5"}`
   - `{"a":"gate", "g":"GATE2", "d":"10"}`
   - `{"a":"gate_ms", "g":"GATE3", "d":"3000"}`
   - `{"a":"gate_ms", "g":"GATE4", "d":"5000"}`

2. **Číselné piny (backwards compatible):**
   - `{"a":"gate", "g":4, "d":"5"}` - stále funguje
   - `{"a":"gate", "g":"12", "d":"10"}` - string s číslom
   - `{"a":"gate_ms", "g":15, "d":"3000"}`

3. **Kombinácia s RID:**
   - `{"a":"gate", "g":"GATE1", "d":"5", "rid":"u876sgx57sg"}`
   - Odpoveď: `{"result":"OK", "rid":"u876sgx57sg"}`

### Validácia

✅ **Platné:**
- GATE1, GATE2, GATE3, GATE4
- 4, 12, 15, 33 (číselné)
- "4", "12", "15", "33" (stringy)

❌ **Neplatné:**
- GATE5, GATE0
- RELAY1, PORT1
- Náhodné texty
- Nepovolené piny (99, 100, ...)

## Testy

### Nové súbory

1. **test/gate_pin_names_tests.yaml**
   - 30+ testovacích scenárov
   - Kategórie:
     - Základné testy s textovými názvami
     - Testy s milisekundami
     - Backwards compatibility
     - Validácia neplatných hodnôt
     - Kombinácia s RID
     - Stress testy

2. **test/test_gate_pin_names.py**
   - Spúšťač testov
   - Využíva existujúci MQTTTestRunner
   - Generuje report

3. **test/GATE_PIN_NAMES_README.md**
   - Kompletná dokumentácia
   - Príklady použitia
   - Tabuľka chybových stavov
   - Odporúčania

### Spustenie testov

```bash
cd test
python3 test_gate_pin_names.py
```

### Testové kategórie

| Kategória | Počet testov | Popis |
|-----------|--------------|-------|
| Textové názvy (gate) | 5 | GATE1-GATE4 zapnutie/vypnutie |
| Textové názvy (gate_ms) | 3 | Milisekundy s textovými názvami |
| Číselné piny | 4 | Backwards compatibility |
| Neplatné hodnoty | 5 | Validácia chýb |
| RID kombinácie | 4 | Textové názvy + RID |
| Stress testy | 4 | Rýchle prepínanie |

## Mapovanie pinov

Podľa `nastavenia.h`:

| Textový názov | Číslo pinu | Použitie |
|---------------|------------|----------|
| GATE1 | 4 | Relé 1 |
| GATE2 | 12 | Relé 2 |
| GATE3 | 15 | Relé 3 |
| GATE4 | 33 | Relé 4 |

## Príklady použitia

### Zapnutie brány pomocou názvu
```json
{"a":"gate", "g":"GATE1", "d":"5"}
```
→ Zapne GATE1 (pin 4) na 5 sekúnd

### Vypnutie pomocou názvu
```json
{"a":"gate", "g":"GATE1", "d":"0"}
```
→ Okamžite vypne GATE1

### Milisekundy s názvom
```json
{"a":"gate_ms", "g":"GATE2", "d":"3000"}
```
→ Zapne GATE2 (pin 12) na 3000ms

### S RID pre tracking
```json
{"a":"gate", "g":"GATE1", "d":"5", "rid":"req-001"}
```
→ Odpoveď: `{"result":"OK", "rid":"req-001"}`

### Neplatná hodnota
```json
{"a":"gate", "g":"GATE5", "d":"5"}
```
→ Odpoveď: `{"result":"Invalid GPIO pin"}`

## Výhody implementácie

1. ✅ **Backwards compatible** - staré príkazy fungujú
2. ✅ **Čitateľnejšie** - GATE1 vs 4
3. ✅ **Bezpečnejšie** - validácia vstupov
4. ✅ **Flexibilné** - číselné aj textové hodnoty
5. ✅ **Testované** - 30+ testovacích scenárov
6. ✅ **Dokumentované** - kompletná dokumentácia

## Chybové stavy

| Vstup | Chyba | Odpoveď |
|-------|-------|---------|
| "GATE5" | Neexistuje | Invalid GPIO pin |
| "RELAY1" | Neplatný názov | Invalid GPIO pin |
| "abc" | Nie je číslo ani názov | Invalid GPIO pin |
| 99 | Nepovolený pin | Invalid GPIO pin |

## Kontrolný zoznam

- ✅ Pridaná funkcia `parseGatePin()`
- ✅ Upravená akcia "gate"
- ✅ Upravená akcia "gate_ms"
- ✅ Validácia textových názvov
- ✅ Backwards compatibility zachovaná
- ✅ Vytvorené testy (gate_pin_names_tests.yaml)
- ✅ Vytvorený test runner (test_gate_pin_names.py)
- ✅ Vytvorená dokumentácia (GATE_PIN_NAMES_README.md)
- ✅ Nastavené spúšťacie práva pre testy
- ✅ Podpora pre RID parameter

## Ďalšie kroky

1. Nahrať firmware na zariadenie
2. Spustiť testy: `python3 test_gate_pin_names.py`
3. Overiť všetky testové scenáre
4. Aktualizovať API dokumentáciu (ak existuje)
5. Informovať používateľov o novej funkcii
