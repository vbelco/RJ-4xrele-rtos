#!/usr/bin/env python3
"""
MQTT Test Runner pre ESP32 riadiacu jednotku
Automatizované testovanie príkazov cez MQTT
"""

import paho.mqtt.client as mqtt
import json
import yaml
import time
import sys
import re
import os
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any, Optional
import ssl

class Colors:
    """ANSI farby pre terminál"""
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

class TestResult:
    """Výsledok jedného testu"""
    def __init__(self, test_id: str, name: str, passed: bool, 
                 command: Dict, expected: Any, actual: Any, 
                 duration: float, error: str = None):
        self.test_id = test_id
        self.name = name
        self.passed = passed
        self.command = command
        self.expected = expected
        self.actual = actual
        self.duration = duration
        self.error = error
        self.timestamp = datetime.now()

class MQTTTestRunner:
    """Hlavná trieda pre spúšťanie testov"""
    
    def __init__(self, config_file: str = "test_config.yaml", save_individual_report: bool = True):
        self.config_file = config_file
        self.config = None
        self.client = None
        self.last_message = None
        self.message_received = False
        self.results: List[TestResult] = []
        self.connected = False
        self.save_individual_report = save_individual_report
        self.stored_values = {}  # Pre uloženie hodnôt počas testov
        
    def load_env(self):
        """Načítanie .env súboru"""
        env_file = Path(__file__).parent / '.env'
        if not env_file.exists():
            return {}
        
        env_vars = {}
        with open(env_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    key, value = line.split('=', 1)
                    # Konverzia true/false na boolean
                    if value.lower() == 'true':
                        value = True
                    elif value.lower() == 'false':
                        value = False
                    # Konverzia čísel
                    elif value.isdigit():
                        value = int(value)
                    env_vars[key] = value
        return env_vars
        
    def load_config(self):
        """Načítanie konfiguračného súboru"""
        try:
            with open(self.config_file, 'r', encoding='utf-8') as f:
                self.config = yaml.safe_load(f)
            
            # Načítaj .env a použij ako default hodnoty pre mqtt sekciu
            env_vars = self.load_env()
            if env_vars and 'mqtt' in self.config:
                mqtt_config = self.config['mqtt']
                # Mapovanie ENV premenných na YAML kľúče
                mqtt_config.setdefault('broker', env_vars.get('MQTT_BROKER'))
                mqtt_config.setdefault('port', env_vars.get('MQTT_PORT'))
                mqtt_config.setdefault('username', env_vars.get('MQTT_USERNAME'))
                mqtt_config.setdefault('password', env_vars.get('MQTT_PASSWORD'))
                mqtt_config.setdefault('use_tls', env_vars.get('MQTT_USE_TLS'))
                mqtt_config.setdefault('ca_cert', env_vars.get('MQTT_CA_CERT'))
                mqtt_config.setdefault('topic_send', env_vars.get('MQTT_TOPIC_SEND'))
                mqtt_config.setdefault('topic_receive', env_vars.get('MQTT_TOPIC_RECEIVE'))
                mqtt_config.setdefault('timeout', env_vars.get('MQTT_TIMEOUT'))
            
            print(f"{Colors.GREEN}✓ Konfigurácia načítaná{Colors.RESET}")
            return True
        except FileNotFoundError:
            print(f"{Colors.RED}✗ Súbor {self.config_file} nebol najdený{Colors.RESET}")
            return False
        except yaml.YAMLError as e:
            print(f"{Colors.RED}✗ Chyba v YAML súbore: {e}{Colors.RESET}")
            return False
    
    def on_connect(self, client, userdata, flags, rc):
        """Callback pri pripojení na MQTT broker"""
        if rc == 0:
            self.connected = True
            topic = self.config['mqtt']['topic_receive']
            client.subscribe(topic)
            print(f"{Colors.GREEN}✓ Pripojené na MQTT broker{Colors.RESET}")
            print(f"{Colors.BLUE}  Počúvam na: {topic}{Colors.RESET}")
        else:
            print(f"{Colors.RED}✗ Chyba pripojenia: {rc}{Colors.RESET}")
            self.connected = False
    
    def on_message(self, client, userdata, msg):
        """Callback pri prijatí správy"""
        try:
            payload = msg.payload.decode('utf-8')
            self.last_message = payload
            self.message_received = True
            
            if self.config.get('test_execution', {}).get('verbose', True):
                print(f"{Colors.BLUE}  ← Odpoveď: {payload[:100]}...{Colors.RESET}" 
                      if len(payload) > 100 else 
                      f"{Colors.BLUE}  ← Odpoveď: {payload}{Colors.RESET}")
        except Exception as e:
            print(f"{Colors.RED}  Chyba pri spracovaní správy: {e}{Colors.RESET}")
    
    def connect_mqtt(self) -> bool:
        """Pripojenie na MQTT broker"""
        try:
            mqtt_config = self.config['mqtt']
            
            self.client = mqtt.Client()
            self.client.on_connect = self.on_connect
            self.client.on_message = self.on_message
            
            # Nastavenie TLS ak je potrebné
            if mqtt_config.get('use_tls', False):
                ca_cert = mqtt_config.get('ca_cert', None)
                if ca_cert:
                    # Použitie custom CA certifikátu (napr. self-signed)
                    self.client.tls_set(ca_certs=ca_cert,
                                       cert_reqs=ssl.CERT_REQUIRED,
                                       tls_version=ssl.PROTOCOL_TLS)
                else:
                    # Použitie systémových certifikátov
                    self.client.tls_set(cert_reqs=ssl.CERT_REQUIRED,
                                       tls_version=ssl.PROTOCOL_TLS)
            
            # Nastavenie prihlasovania
            if mqtt_config.get('username') and mqtt_config.get('password'):
                self.client.username_pw_set(mqtt_config['username'], 
                                           mqtt_config['password'])
            
            # Pripojenie
            print(f"\n{Colors.BOLD}Pripájanie na MQTT broker...{Colors.RESET}")
            print(f"  Broker: {mqtt_config['broker']}:{mqtt_config['port']}")
            
            self.client.connect(mqtt_config['broker'], mqtt_config['port'], 60)
            self.client.loop_start()
            
            # Čakanie na pripojenie
            timeout = 10
            for i in range(timeout):
                if self.connected:
                    return True
                time.sleep(1)
            
            print(f"{Colors.RED}✗ Timeout pri pripojení{Colors.RESET}")
            return False
            
        except Exception as e:
            print(f"{Colors.RED}✗ Chyba pripojenia: {e}{Colors.RESET}")
            return False
    
    def send_command(self, command: Dict) -> Optional[str]:
        """Odoslanie príkazu a čakanie na odpoveď"""
        try:
            self.message_received = False
            self.last_message = None
            
            # Konverzia na JSON
            json_command = json.dumps(command)
            
            # Odoslanie
            topic = self.config['mqtt']['topic_send']
            self.client.publish(topic, json_command)
            
            if self.config.get('test_execution', {}).get('verbose', True):
                print(f"{Colors.BLUE}  → Príkaz: {json_command}{Colors.RESET}")
            
            # Čakanie na odpoveď
            timeout = self.config['mqtt'].get('timeout', 5)
            for i in range(int(timeout * 10)):
                if self.message_received:
                    return self.last_message
                time.sleep(0.1)
            
            return None
            
        except Exception as e:
            print(f"{Colors.RED}  Chyba odosielania: {e}{Colors.RESET}")
            return None
    
    def validate_response(self, response: str, expected: Any, 
                         expected_type: str) -> tuple[bool, str]:
        """Validácia odpovede podľa očakávaného typu"""
        if response is None:
            return False, "Žiadna odpoveď (timeout)"
        
        try:
            if expected_type == "exact":
                # Presná zhoda
                passed = response == expected
                error = None if passed else f"Očakávané: '{expected}', Dostané: '{response}'"
                
            elif expected_type == "contains":
                # Obsahuje reťazec
                passed = expected in response
                error = None if passed else f"Odpoveď neobsahuje: '{expected}'"
                
            elif expected_type == "regex":
                # Regex match
                passed = bool(re.search(expected, response))
                error = None if passed else f"Regex '{expected}' sa nezhoduje"
                
            elif expected_type == "json_key":
                # Kontrola kľúča v JSON odpovedi
                try:
                    json_response = json.loads(response)
                    passed = expected in json_response
                    error = None if passed else f"JSON neobsahuje kľúč: '{expected}'"
                except json.JSONDecodeError:
                    passed = False
                    error = "Odpoveď nie je platný JSON"
            else:
                passed = False
                error = f"Neznámy typ validácie: {expected_type}"
                
            return passed, error
            
        except Exception as e:
            return False, f"Chyba validácie: {str(e)}"
    
    def run_test(self, test: Dict) -> TestResult:
        """Spustenie jedného testu"""
        test_id = test.get('id', 'unknown')
        name = test.get('name', 'Unnamed test')
        
        print(f"\n{Colors.BOLD}[{test_id}] {name}{Colors.RESET}")
        
        start_time = time.time()
        
        # Odoslanie príkazu
        response = self.send_command(test['command'])
        
        # Validácia odpovede
        expected = test.get('expected', '')
        expected_type = test.get('expected_type', 'exact')
        
        passed, error = self.validate_response(response, expected, expected_type)
        
        duration = time.time() - start_time
        
        # Výsledok
        result = TestResult(
            test_id=test_id,
            name=name,
            passed=passed,
            command=test['command'],
            expected=expected,
            actual=response,
            duration=duration,
            error=error
        )
        
        # Výpis výsledku
        if passed:
            print(f"{Colors.GREEN}  ✓ PASSED{Colors.RESET} ({duration:.2f}s)")
        else:
            print(f"{Colors.RED}  ✗ FAILED{Colors.RESET} ({duration:.2f}s)")
            if error:
                print(f"{Colors.RED}    {error}{Colors.RESET}")
        
        # Čakanie po teste ak je definované
        wait_after = test.get('wait_after', 0)
        if wait_after > 0:
            print(f"{Colors.YELLOW}  ⏱ Čakám {wait_after}s...{Colors.RESET}")
            time.sleep(wait_after)
        
        return result
    
    def run_all_tests(self) -> bool:
        """Spustenie všetkých testov"""
        tests = self.config.get('tests', [])
        test_execution = self.config.get('test_execution', {})
        
        print(f"\n{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"{Colors.BOLD}Spúšťam {len(tests)} testov...{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}")
        
        for test in tests:
            # Preskočenie vypnutých testov
            if not test.get('enabled', True):
                print(f"\n{Colors.YELLOW}[{test.get('id', '?')}] {test.get('name', '')} - SKIPPED{Colors.RESET}")
                continue
            
            # Spustenie testu
            result = self.run_test(test)
            self.results.append(result)
            
            # Zastavenie pri zlyhaní ak je nastavené
            if not result.passed and test_execution.get('stop_on_fail', False):
                print(f"\n{Colors.RED}Test zlyhal, zastavujem vykonávanie testov.{Colors.RESET}")
                break
            
            # Delay medzi testami
            delay = test_execution.get('delay_between_tests', 0.5)
            time.sleep(delay)
        
        return True
    
    def print_summary(self):
        """Výpis súhrnu testov"""
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed
        
        print(f"\n{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"{Colors.BOLD}SUMÁR TESTOV{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"Celkovo testov: {total}")
        print(f"{Colors.GREEN}Úspešných: {passed}{Colors.RESET}")
        print(f"{Colors.RED}Neúspešných: {failed}{Colors.RESET}")
        
        if failed > 0:
            print(f"\n{Colors.RED}Neúspešné testy:{Colors.RESET}")
            for result in self.results:
                if not result.passed:
                    print(f"  {Colors.RED}✗ [{result.test_id}] {result.name}{Colors.RESET}")
                    if result.error:
                        print(f"    {result.error}")
        
        total_duration = sum(r.duration for r in self.results)
        print(f"\nCelkový čas: {total_duration:.2f}s")
        print(f"{'='*60}\n")
    
    def save_report(self):
        """Uloženie reportu do súboru"""
        if not self.save_individual_report:
            return
        
        test_execution = self.config.get('test_execution', {})
        
        if not test_execution.get('save_report', True):
            return
        
        report_format = test_execution.get('report_format', 'html')
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        
        if report_format == 'html':
            self.save_html_report(timestamp)
        elif report_format == 'json':
            self.save_json_report(timestamp)
        else:
            self.save_text_report(timestamp)
    
    def save_html_report(self, timestamp: str):
        """Uloženie HTML reportu"""
        filename = f"test_report_{timestamp}.html"
        
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed
        success_rate = (passed / total * 100) if total > 0 else 0
        
        html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Test Report - {timestamp}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        h1 {{ color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }}
        .summary {{ background: #e8f5e9; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        .summary-grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }}
        .summary-item {{ text-align: center; }}
        .summary-item .value {{ font-size: 32px; font-weight: bold; }}
        .summary-item .label {{ color: #666; font-size: 14px; }}
        .passed {{ color: #4CAF50; }}
        .failed {{ color: #f44336; }}
        table {{ width: 100%; border-collapse: collapse; margin-top: 20px; }}
        th {{ background: #4CAF50; color: white; padding: 12px; text-align: left; }}
        td {{ padding: 10px; border-bottom: 1px solid #ddd; }}
        tr:hover {{ background: #f5f5f5; }}
        .test-passed {{ background: #e8f5e9; }}
        .test-failed {{ background: #ffebee; }}
        .command {{ font-family: monospace; background: #f5f5f5; padding: 5px; border-radius: 3px; }}
        .error {{ color: #f44336; font-size: 12px; margin-top: 5px; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>Test Report</h1>
        <p>Dátum: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        
        <div class="summary">
            <div class="summary-grid">
                <div class="summary-item">
                    <div class="value">{total}</div>
                    <div class="label">Celkovo testov</div>
                </div>
                <div class="summary-item passed">
                    <div class="value">{passed}</div>
                    <div class="label">Úspešných</div>
                </div>
                <div class="summary-item failed">
                    <div class="value">{failed}</div>
                    <div class="label">Neúspešných</div>
                </div>
                <div class="summary-item">
                    <div class="value">{success_rate:.1f}%</div>
                    <div class="label">Úspešnosť</div>
                </div>
            </div>
        </div>
        
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Názov</th>
                    <th>Príkaz</th>
                    <th>Výsledok</th>
                    <th>Čas</th>
                </tr>
            </thead>
            <tbody>
"""
        
        for result in self.results:
            status_class = "test-passed" if result.passed else "test-failed"
            status_text = "✓ PASSED" if result.passed else "✗ FAILED"
            status_color = "passed" if result.passed else "failed"
            
            # Formátuj command
            if isinstance(result.command, str):
                cmd_display = result.command.replace('\n', '<br>')
            else:
                cmd_display = json.dumps(result.command)
            
            html += f"""
                <tr class="{status_class}">
                    <td><strong>{result.test_id}</strong></td>
                    <td>{result.name}</td>
                    <td><code class="command" style="white-space: pre-wrap;">{cmd_display}</code></td>
                    <td class="{status_color}"><strong>{status_text}</strong>
                        {f'<div class="error">{result.error}</div>' if result.error else ''}
                    </td>
                    <td>{result.duration:.2f}s</td>
                </tr>
"""
        
        html += """
            </tbody>
        </table>
    </div>
</body>
</html>
"""
        
        with open(filename, 'w', encoding='utf-8') as f:
            f.write(html)
        
        print(f"{Colors.GREEN}✓ HTML report uložený: {filename}{Colors.RESET}")
    
    def save_json_report(self, timestamp: str):
        """Uloženie JSON reportu"""
        filename = f"test_report_{timestamp}.json"
        
        report = {
            'timestamp': timestamp,
            'date': datetime.now().isoformat(),
            'summary': {
                'total': len(self.results),
                'passed': sum(1 for r in self.results if r.passed),
                'failed': sum(1 for r in self.results if not r.passed),
            },
            'tests': [
                {
                    'id': r.test_id,
                    'name': r.name,
                    'passed': r.passed,
                    'command': r.command,
                    'expected': r.expected,
                    'actual': r.actual,
                    'duration': r.duration,
                    'error': r.error,
                    'timestamp': r.timestamp.isoformat()
                }
                for r in self.results
            ]
        }
        
        with open(filename, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        
        print(f"{Colors.GREEN}✓ JSON report uložený: {filename}{Colors.RESET}")
    
    def save_text_report(self, timestamp: str):
        """Uloženie textového reportu"""
        filename = f"test_report_{timestamp}.txt"
        
        with open(filename, 'w', encoding='utf-8') as f:
            f.write(f"TEST REPORT - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("="*60 + "\n\n")
            
            total = len(self.results)
            passed = sum(1 for r in self.results if r.passed)
            failed = total - passed
            
            f.write(f"SUMÁR:\n")
            f.write(f"  Celkovo: {total}\n")
            f.write(f"  Úspešných: {passed}\n")
            f.write(f"  Neúspešných: {failed}\n\n")
            f.write("="*60 + "\n\n")
            
            for result in self.results:
                status = "PASSED" if result.passed else "FAILED"
                f.write(f"[{result.test_id}] {result.name}\n")
                f.write(f"  Status: {status}\n")
                f.write(f"  Príkaz: {json.dumps(result.command)}\n")
                f.write(f"  Čas: {result.duration:.2f}s\n")
                if result.error:
                    f.write(f"  Chyba: {result.error}\n")
                f.write("\n")
        
        print(f"{Colors.GREEN}✓ Textový report uložený: {filename}{Colors.RESET}")
    
    def disconnect(self):
        """Odpojenie od MQTT brokera"""
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
            print(f"{Colors.GREEN}✓ Odpojené od MQTT brokera{Colors.RESET}")
    
    
    def get_test_data(self) -> dict:
        """Vráti dáta testov pre unified report"""
        tests = []
        for result in self.results:
            test_dict = {
                'id': result.test_id,
                'name': result.name,
                'status': 'passed' if result.passed else ('skipped' if hasattr(result, 'skipped') and result.skipped else 'failed'),
                'duration': result.duration,
                'command': str(result.command),
                'response': result.actual if result.actual else '',
            }
            if not result.passed and result.error:
                test_dict['error'] = result.error
            tests.append(test_dict)
        
        passed_count = sum(1 for r in self.results if r.passed)
        failed_count = sum(1 for r in self.results if not r.passed)
        
        return {
            'total': len(self.results),
            'passed': passed_count,
            'failed': failed_count,
            'tests': tests
        }
    
    def run(self) -> int:
        """Hlavná funkcia - spustenie celého testu"""
        print(f"\n{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"{Colors.BOLD}ESP32 MQTT Test Runner{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}\n")
        
        # Načítanie konfigurácie
        if not self.load_config():
            return 1
        
        # Pripojenie na MQTT
        if not self.connect_mqtt():
            return 1
        
        try:
            # Spustenie testov
            self.run_all_tests()
            
            # Výpis súhrnu
            self.print_summary()
            
            # Uloženie reportu
            self.save_report()
            
            # Návratový kód podľa výsledkov
            failed_count = sum(1 for r in self.results if not r.passed)
            return 0 if failed_count == 0 else 1
            
        finally:
            # Odpojenie
            self.disconnect()

def main():
    """Entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='MQTT Test Runner pre ESP32')
    parser.add_argument('-c', '--config', default='test_config.yaml',
                       help='Cesta ku konfiguračnému súboru (default: test_config.yaml)')
    parser.add_argument('-v', '--version', action='version', version='1.0.0')
    
    args = parser.parse_args()
    
    runner = MQTTTestRunner(args.config)
    exit_code = runner.run()
    
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
