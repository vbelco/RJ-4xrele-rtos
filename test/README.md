# ESP32 Automatizované Testy

Tento adresár obsahuje automatizované testy pre ESP32 riadiacu jednotku - **MQTT** aj **HTTP API** testy.

## 📁 Štruktúra

```
test/
├── mqtt_test_runner.py      # MQTT testovací skript
├── test_config.yaml          # MQTT konfigurácia a testovacie prípady
├── http_test_runner.py      # HTTP API testovací skript
├── http_test_config.yaml    # HTTP API konfigurácia a testovacie prípady
├── run_all_tests.py         # Spustí MQTT aj HTTP testy naraz
├── requirements.txt          # Python závislosti
└── README.md                # Táto dokumentácia
```

## 🚀 Rýchly Štart

### 1. Inštalácia závislostí

```bash
cd test
pip install -r requirements.txt
```

Alebo ak používaš Python 3:
```bash
pip3 install -r requirements.txt
```

### 2. Konfigurácia

**DÔLEŽITÉ: Nastavenie .env súboru**

Všetky citlivé konfiguračné údaje (MQTT broker, heslá, IP adresy) sú uložené v `.env` súbore, ktorý **NIE JE** súčasťou git repozitára.

1. **Vytvorenie .env súboru:**
   ```bash
   cp .env.example .env
   ```

2. **Úprava .env súboru:**
   Otvor `.env` súbor a uprav hodnoty podľa tvojho nastavenia:
   ```bash
   # MQTT Broker Configuration
   MQTT_BROKER=dev4.ccsipro.sk        # Tvoj MQTT broker
   MQTT_PORT=8883                      # Port (8883 pre TLS)
   MQTT_USERNAME=prototip              # Tvoje používateľské meno
   MQTT_PASSWORD=tvoje_heslo           # Tvoje heslo
   MQTT_USE_TLS=true                   # true/false
   MQTT_CA_CERT=broker.crt             # Cesta k certifikátu
   MQTT_TOPIC_SEND=system/vbelco/eth-liligo1
   MQTT_TOPIC_RECEIVE=res/system/vbelco/eth-liligo1
   MQTT_TIMEOUT=5
   
   # HTTP API Configuration
   HTTP_BASE_URL=http://192.168.1.189  # IP adresa tvojho ESP32
   HTTP_TIMEOUT=5
   ```

3. **Voliteľné prepísanie v YAML súboroch:**
   Ak chceš pre konkrétne testy použiť iné hodnoty, môžeš ich prepísať v YAML súboroch:
   
   - `test_config.yaml` - MQTT testy
   - `http_test_config.yaml` - HTTP testy
   - `gate_pin_names_tests.yaml` - Testy GATE názvov
   - `gate_timing_tests.yaml` - Testy časovania (môže mať iný timeout)

**Poznámka:** `.env` súbor je automaticky ignorovaný git-om (.gitignore), takže tvoje heslá zostanú v bezpečí.

### 3. Spustenie testov

**Všetky testy naraz (MQTT + HTTP):**
```bash
python run_all_tests.py
```

**Len MQTT testy:**
```bash
python mqtt_test_runner.py
# alebo
python run_all_tests.py --mqtt-only
```

**Len HTTP API testy:**
```bash
python http_test_runner.py
# alebo
python run_all_tests.py --http-only
```

**S vlastnou konfiguráciou:**
```bash
python mqtt_test_runner.py -c moja_mqtt_konfig.yaml
python http_test_runner.py -c moja_http_konfig.yaml
```

## 📝 Ako fungujú testy

### Proces testovania

1. **Načítanie konfigurácie** - zo súboru `test_config.yaml`
2. **Pripojenie na MQTT broker** - s TLS a autentifikáciou
3. **Spustenie testov** - postupne podľa poradia v config súbore
4. **Validácia odpovedí** - kontrola či odpoveď zodpovedá očakávaniu
5. **Generovanie reportu** - HTML, JSON alebo textový súbor

### Typy validácie

V `test_config.yaml` môžete pre každý test definovať typ validácie:

- **`exact`** - presná zhoda
  ```yaml
  expected: "pingOK"
  expected_type: "exact"
  ```

- **`contains`** - odpoveď obsahuje text
  ```yaml
  expected: "gpio"
  expected_type: "contains"
  ```

- **`regex`** - regex pattern
  ```yaml
  expected: "gpio.*4"
  expected_type: "regex"
  ```

- **`json_key`** - kontrola kľúča v JSON odpovedi
  ```yaml
  expected: "version"
  expected_type: "json_key"
  ```

## ✏️ Pridanie vlastných testov

Otvorte `test_config.yaml` a pridajte nový test do sekcie `tests:`:

```yaml
tests:
  - id: TC99
    name: "Môj nový test"
    command:
      a: "gate"
      g: "4"
      d: "10"
    expected_type: "contains"
    expected: "success"
    wait_after: 2  # voliteľné čakanie po teste
```

### Paramettre testu:

- **`id`** - unikátny identifikátor (napr. TC01, TC02...)
- **`name`** - popis testu
- **`command`** - JSON príkaz na odoslanie
- **`expected`** - očakávaná odpoveď
- **`expected_type`** - typ validácie (exact, contains, regex, json_key)
- **`wait_after`** - (voliteľné) sekundy čakania po teste
- **`enabled`** - (voliteľné) false = test sa preskočí

## 📊 Reporty

Po skončení testov sa automaticky vygeneruje report (podľa nastavenia v config):

### HTML Report
```yaml
test_execution:
  save_report: true
  report_format: "html"
```
Vytvorí pekný prehľadný HTML súbor s výsledkami.

### JSON Report
```yaml
test_execution:
  report_format: "json"
```
Vhodný pre ďalšie programové zpracovanie.

### Textový Report
```yaml
test_execution:
  report_format: "text"
```
Jednoduchý textový súbor.

## 🔧 Nastavenia vykonávania testov

V `test_config.yaml` sekcia `test_execution`:

```yaml
test_execution:
  stop_on_fail: false           # true = zastav pri prvom zlyhaní
  delay_between_tests: 0.5      # sekundy medzi testami
  verbose: true                 # detailný výpis do konzoly
  save_report: true             # uloží report do súboru
  report_format: "html"         # html, json, text
```

## 🐛 Riešenie problémov

### Chyba pripojenia na MQTT

```
✗ Chyba pripojenia na MQTT broker
```

**Riešenie:**
- Skontrolujte či je ESP32 zapnuté a pripojené
- Overte broker adresu a port v `test_config.yaml`
- Skontrolujte používateľské meno a heslo
- Overte či máte funkčné sieťové pripojenie

### Timeout pri čakaní na odpoveď

```
Žiadna odpoveď (timeout)
```

**Riešenie:**
- Zvýšte timeout v config súbore:
  ```yaml
  mqtt:
   🌐 HTTP API Testy

### Čo testujú HTTP API testy?

HTTP testy kontrolujú všetky API endpointy ESP32:

- **GET /api** - GET requesty s JSON príkazmi
- **POST /api** - POST requesty s JSON príkazmi
- **GET /getParams** - Získanie konfigurácie
- **GET /setParams** - Nastavenie parametrov (defaultne vypnuté)

### Príklad HTTP testu

```yaml
- id: HTTP10
  name: "GET /api - ping test"
  method: GET
  endpoint: "/api"
  data:
    a: "ping"
  expected: "result"
  expected_type: "json_key"
```

### ⚠️ DÔLEŽITÉ - Nastavenie IP adresy

Pred spustením HTTP testov **MUSÍŠ** upraviť IP adresu v `http_test_config.yaml`:

```yaml
http:
  host: "192.168.1.100"  # <-- UPRAV NA IP ADRESU TVOJHO ESP32!
  port: 9090
```

Ako zistiť IP adresu ESP32?
1. Pozri na Serial Monitor pri štarte ESP32
2. Alebo použij MQTT príkaz `{"a":"status"}`

## 📚 Príklady použitia

### Spustenie všetkých testov

```bash
cd test
python run_all_tests.py
```

### Test len MQTTte či ESP32 reaguje na príkazy
- Overte topic names (musia sa zhodovať s ESP32)

### Import error - paho.mqtt

```
ModuleNotFoundError: No module named 'paho'
```

**Riešenie:**
```bash
pip Test len HTTP API endpointov

Vytvorte `quick_http_test.yaml`:
```yaml
http:
  host: "192.168.1.100"
  port: 9090
  timeout: 5

tests:
  - id: HTTP01
    name: "Ping test"
    method: POST
    endpoint: "/api"
    data: {a: "ping"}
    expected: 200
    expected_type: "status_code"
    
  - id: HTTP02
    name: "Status"
    method: GET
    endpoint: "/api"
    data: {a: "status"}
    expected: 200
    expected_type: "status_code"

test_execution:
  verbose: true
  save_report: true
```

Spustenie:
```bash
python http_test_runner.py -c quick_http_test.yaml
```

### Continuous Integration (CI/CD)

Pre použitie v CI/CD pipeline:

```bash
#!/bin/bash
cd test
pip install -r requirements.txt

### MQTT testy problémy

1. Skontrolujte či ESP32 beží a je pripojené na MQTT
2. Zapnite verbose mode v test_config.yaml
3. Sledujte Serial Monitor ESP32 pre detailné informácie
4. Overte MQTT pripojenie pomocou mosquitto_sub:
   ```bash
   mosquitto_sub -h dev4.ccsipro.sk -p 8883 --capath /etc/ssl/certs \
     -u prototip -P Rff5Gggs899 \
     -t "res/system/vbelco/eth-liligo1" -v
   ```

### HTTP API testy problémy

1. **Chyba pripojenia:**
   ```
   ✗ Chyba pripojenia - skontroluj IP adresu a port
   ```
   **Riešenie:**
   - Skontroluj IP adresu v `http_test_config.yaml`
   - Ping ESP32: `ping 192.168.1.100`
   - Overte či API server beží (Serial Monitor)
   - Skontroluj či ESP32 má IP adresu

2. **Timeout:**
   ```
   ✗ Timeout - ESP32 neodpovedá
   ```
   **Riešenie:**
   - Zvýš timeout v config súbore
   - Skontroluj firewall
   - Overte či port 9090 je otvorený

3. **Test cez curl:**
   ```bash
   # Základný test
   curl http://192.168.1.100:9090/getParams
   
   # POST API test
   curl -X POST http://192.168.1.100:9090/api \
     -H "Content-Type: application/json" \
     -d '{"a":"ping"}'
   
   # GET API test
   curl "http://192.168.1.100:9090/api?request=%7B%22a%22%3A%22ping%22%7D"
```

Alebo oddelene:
```bash
#!/bin/bash
cd test
pip install -r requirements.txt

# MQTT testy
python mqtt_test_runner.py
MQTT_EXIT=$?

# HTTP testy
python http_test_runner.py
HTTP_EXIT=$?

# Kontrola výsledkov
if [ $MQTT_EXIT -eq 0 ] && [ $HTTP_EXIT -eq 0 ]; then
    echo "All tests passed!"
    exit 0
  password: "Rff5Gggs899"
  use_tls: true
  topic_send: "system/vbelco/eth-liligo1"
  topic_receive: "res/system/vbelco/eth-liligo1"
  timeout: 5

tests:
  - id: TC01
    name: "Ping test"
    command: {a: "ping"}
    expected: "pingOK"
    expected_type: "exact"
    
  - id: TC02
    name: "Status"
    command: {a: "status"}
    expected_type: "json_key"
    expected: "version"

test_execution:
  stop_on_fail: true
  delay_between_tests: 0.5
  verbose: true
  save_report: true
  report_format: "html"
```

Spustenie:
```bash
python mqtt_test_runner.py -c quick_test.yaml
```

### Continuous Integration (CI/CD)

Pre použitie v CI/CD pipeline:

```bash
#!/bin/bash
cd test
pip install -r requirements.txt
python mqtt_test_runner.py

# Exit code: 0 = všetky testy ok, 1 = niektoré zlyhali
if [ $? -eq 0 ]; then
    echo "All tests passed!"
else
    echo "Some tests failed!"
    exit 1
fi
```

## 💡 Tipy a Triky

1. **Vypínanie testov** - Test môžete dočasne vypnúť:
   ```yaml
   - id: TC40
     enabled: false  # tento test sa preskočí
     name: "Nebezpečný test"
   ```

2. **Delay medzi testami** - Ak testujete časové správanie, nastavte delay:
   ```yaml
   wait_after: 5  # počká 5 sekúnd po tomto teste
   ```

3. **Verbose mode** - Pre ladenie zapnite detailný výpis:
   ```yaml
   test_execution:
     verbose: true
   ```

4. **Testovanie rôznych prostredí** - Vytvorte viacero config súborov:
   - `test_config_dev.yaml` - vývojové prostredie
   - `test_config_prod.yaml` - produkčné prostredie

## 📞 Pomoc

Ak máte problémy:

1. Skontrolujte či ESP32 beží a je pripojené na MQTT
2. Zapnite verbose mode v test_config.yaml
3. Sledujte Serial Monitor ESP32 pre detailné informácie
4. Overte MQTT pripojenie pomocou mosquitto_sub:
   ```bash
   mosquitto_sub -h dev4.ccsipro.sk -p 8883 --capath /etc/ssl/certs \
     -u prototip -P Rff5Gggs899 \
     -t "res/system/vbelco/eth-liligo1" -v
   ```

## 📄 Licencia

Použite podľa potreby pre váš projekt.
