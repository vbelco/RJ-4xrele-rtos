# Textové názvy pinov pre Gate príkazy

## Prehľad

Okrem číselných hodnôt pinov (napr. `"g": 4`) teraz podporujeme aj textové názvy pinov:
- `GATE1` = pin 4
- `GATE2` = pin 12
- `GATE3` = pin 15
- `GATE4` = pin 33

## Použitie

### Príklady príkazov

#### Gate (sekundy)

**Textové názvy:**
```json
{"a":"gate", "g":"GATE1", "d":"5"}
{"a":"gate", "g":"GATE2", "d":"10"}
{"a":"gate", "g":"GATE3", "d":"0"}
{"a":"gate", "g":"GATE4", "d":"11111"}
```

**Číselné piny (stále podporované):**
```json
{"a":"gate", "g":4, "d":"5"}
{"a":"gate", "g":12, "d":"10"}
```

#### Gate_ms (milisekundy)

**Textové názvy:**
```json
{"a":"gate_ms", "g":"GATE1", "d":"3000"}
{"a":"gate_ms", "g":"GATE2", "d":"5000"}
{"a":"gate_ms", "g":"GATE3", "d":"0"}
```

**Číselné piny:**
```json
{"a":"gate_ms", "g":4, "d":"3000"}
{"a":"gate_ms", "g":15, "d":"5000"}
```

#### S RID parametrom

```json
{"a":"gate", "g":"GATE1", "d":"5", "rid":"u876sgx57sg"}
{"a":"gate_ms", "g":"GATE2", "d":"3000", "rid":"abc123"}
```

**Odpoveď:**
```json
{"result":"OK", "rid":"u876sgx57sg"}
```

## Validácia

Systém validuje názvy pinov:

✅ **Platné hodnoty:**
- `"GATE1"`, `"GATE2"`, `"GATE3"`, `"GATE4"` (textové)
- `4`, `12`, `15`, `33` (číselné)
- Stringy obsahujúce čísla: `"4"`, `"12"`, `"15"`, `"33"`

❌ **Neplatné hodnoty:**
- `"GATE5"`, `"GATE0"`
- `"RELAY1"`, `"PORT1"`
- Náhodné texty: `"abc"`, `"xyz"`
- Nepovolené číselné piny: `99`, `100`
- Prázdne hodnoty

**Chybová odpoveď:**
```json
{"result":"Invalid GPIO pin"}
```

alebo s RID:
```json
{"result":"Invalid GPIO pin", "rid":"u876sgx57sg"}
```

## Backwards Compatibility

Všetky existujúce príkazy s číselnými pinmi fungujú bez zmeny:
```json
{"a":"gate", "g":4, "d":"5"}      // Stále funguje
{"a":"gate", "g":"4", "d":"5"}    // Stále funguje
{"a":"gate", "g":"GATE1", "d":"5"}  // Nová funkcionalita
```

## Implementácia

### Funkcia parseGatePin()

Funkcia `parseGatePin()` v [processJSON.ino](../processJSON.ino) konvertuje:
- Textové názvy na čísla pinov
- Číselné hodnoty ponecháva bez zmeny
- Stringové čísla konvertuje na int
- Neplatné hodnoty vracia ako -1

```cpp
int parseGatePin(JsonVariant gateValue) {
  // Číselná hodnota
  if (gateValue.is<int>()) {
    return gateValue.as<int>();
  }
  
  // Textový názov
  if (gateValue.is<const char*>()) {
    String gateName = gateValue.as<String>();
    if (gateName == "GATE1") return GATE1;
    if (gateName == "GATE2") return GATE2;
    if (gateName == "GATE3") return GATE3;
    if (gateName == "GATE4") return GATE4;
    // Fallback na číselný string
    return gateName.toInt();
  }
  
  return -1; // Neplatná hodnota
}
```

## Testovanie

### Spustenie testov

```bash
cd test
python3 test_gate_pin_names.py
```

### Testovací súbor

Testy sú v súbore `gate_pin_names_tests.yaml` a zahŕňajú:

1. **Základné testy textových názvov** (GATE1-GATE4)
2. **Testy s milisekundami** (gate_ms)
3. **Backwards compatibility** (číselné piny)
4. **Validácia neplatných hodnôt**
5. **Kombinácia s RID parametrom**
6. **Stress testy** (rýchle prepínanie)

### Príklad testu

```yaml
- id: GATE_NAME_01
  name: "Gate GATE1 - zapnutie pomocou textového názvu"
  command:
    a: "gate"
    g: "GATE1"
    d: "5"
  expected_type: "contains"
  expected: "OK"
```

### Očakávané výsledky

```
✓ GATE_NAME_01: Gate GATE1 - zapnutie pomocou textového názvu
✓ GATE_NAME_02: Gate GATE2 - zapnutie pomocou textového názvu
✓ GATE_INVALID_01: Gate - neplatný textový názov GATE5
✓ GATE_RID_01: Gate GATE1 s RID - textový názov
```

## Konfigurácia pinov

Definície pinov v [nastavenia.h](../nastavenia.h):

```cpp
#define GATE1 4   // relé 
#define GATE2 12  // relé 
#define GATE3 15  // relé 
#define GATE4 33  // relé 
```

## Výhody textových názvov

1. **Lepšia čitateľnosť** - `"GATE1"` je zrozumiteľnejšie ako `4`
2. **Menej chýb** - znížené riziko použitia nesprávneho čísla pinu
3. **Abstrakcia** - zmena fyzického pinu nevyžaduje zmenu príkazov
4. **Dokumentácia** - príkazy sú samovysvetľujúce
5. **Backwards compatible** - staré príkazy fungujú

## Chybové stavy

| Vstup | Výsledok | Dôvod |
|-------|----------|-------|
| `"GATE5"` | Invalid GPIO pin | Neexistuje GATE5 |
| `"RELAY1"` | Invalid GPIO pin | Neplatný názov |
| `"abc"` | Invalid GPIO pin | Nie je číslo ani platný názov |
| `99` | Invalid GPIO pin | Pin nie je v zozname povolených |
| `""` | Invalid GPIO pin | Prázdna hodnota |

## Odporúčania

1. **Používajte textové názvy** pre nové projekty
2. **Číselné piny** sú OK pre existujúce skripty
3. **Validujte vstupy** na strane klienta
4. **Testujte** novú funkcionalitu pomocou poskytnutých testov
