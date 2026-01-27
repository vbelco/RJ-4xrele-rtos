#!/usr/bin/env python3
"""
HTTP API Test Runner pre ESP32 riadiacu jednotku
Automatizované testovanie HTTP endpointov
"""

import requests
import json
import yaml
import time
import sys
import os
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any, Optional
from urllib.parse import urlencode, quote

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
                 request: Dict, expected: Any, actual: Any, 
                 duration: float, error: str = None, status_code: int = None):
        self.test_id = test_id
        self.name = name
        self.passed = passed
        self.request = request
        self.expected = expected
        self.actual = actual
        self.duration = duration
        self.error = error
        self.status_code = status_code
        self.timestamp = datetime.now()

class HTTPTestRunner:
    """Hlavná trieda pre spúšťanie HTTP API testov"""
    
    def __init__(self, config_file: str = "http_test_config.yaml", save_individual_report: bool = True):
        self.config_file = config_file
        self.config = None
        self.results: List[TestResult] = []
        self.save_individual_report = save_individual_report
        self.base_url = None
        
    def load_env(self):
        """Načítanie konfigurácie z .env súboru"""
        env_path = Path(__file__).parent / '.env'
        env_vars = {}
        
        if env_path.exists():
            with open(env_path, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#') and '=' in line:
                        key, value = line.split('=', 1)
                        key = key.strip()
                        value = value.strip()
                        
                        # Konverzia typov
                        if value.lower() in ('true', 'false'):
                            value = value.lower() == 'true'
                        elif value.isdigit():
                            value = int(value)
                        
                        env_vars[key] = value
        
        return env_vars
    
    def load_config(self):
        """Načítanie konfiguračného súboru"""
        try:
            # Načítanie .env súboru
            env_vars = self.load_env()
            
            with open(self.config_file, 'r', encoding='utf-8') as f:
                self.config = yaml.safe_load(f)
            
            # Doplnenie HTTP konfigurácie z .env ak nie je v YAML
            if 'http' not in self.config:
                self.config['http'] = {}
            
            # Nastavenie defaultov z .env (YAML má prednosť)
            if 'HTTP_BASE_URL' in env_vars:
                # Parse base URL
                base_url = env_vars['HTTP_BASE_URL']
                if base_url.startswith('http://'):
                    url_parts = base_url.replace('http://', '').split(':')
                    self.config['http'].setdefault('host', url_parts[0])
                    if len(url_parts) > 1:
                        self.config['http'].setdefault('port', int(url_parts[1]))
                    else:
                        self.config['http'].setdefault('port', 80)
            
            self.config['http'].setdefault('timeout', env_vars.get('HTTP_TIMEOUT', 5))
            
            print(f"{Colors.GREEN}✓ Konfigurácia načítaná{Colors.RESET}")
            
            # Zostavenie base URL
            http_config = self.config['http']
            self.base_url = f"http://{http_config['host']}:{http_config['port']}"
            
            return True
        except FileNotFoundError:
            print(f"{Colors.RED}✗ Súbor {self.config_file} nebol najdený{Colors.RESET}")
            return False
        except yaml.YAMLError as e:
            print(f"{Colors.RED}✗ Chyba v YAML súbore: {e}{Colors.RESET}")
            return False
    
    def check_connection(self) -> bool:
        """Kontrola či je ESP32 dostupné"""
        try:
            print(f"\n{Colors.BOLD}Kontrola pripojenia na ESP32...{Colors.RESET}")
            print(f"  URL: {self.base_url}")
            
            timeout = self.config['http'].get('timeout', 5)
            response = requests.get(f"{self.base_url}/getParams", timeout=timeout)
            
            if response.status_code == 200:
                print(f"{Colors.GREEN}✓ ESP32 je dostupné{Colors.RESET}")
                return True
            else:
                print(f"{Colors.RED}✗ ESP32 odpovedalo s kódom: {response.status_code}{Colors.RESET}")
                return False
                
        except requests.exceptions.Timeout:
            print(f"{Colors.RED}✗ Timeout - ESP32 neodpovedá{Colors.RESET}")
            return False
        except requests.exceptions.ConnectionError:
            print(f"{Colors.RED}✗ Chyba pripojenia - skontroluj IP adresu a port{Colors.RESET}")
            return False
        except Exception as e:
            print(f"{Colors.RED}✗ Chyba: {e}{Colors.RESET}")
            return False
    
    def send_get_request(self, endpoint: str, params: Dict = None) -> tuple:
        """Odoslanie GET requestu"""
        try:
            url = f"{self.base_url}{endpoint}"
            timeout = self.config['http'].get('timeout', 5)
            
            if self.config.get('test_execution', {}).get('verbose', True):
                print(f"{Colors.BLUE}  → GET {endpoint}{Colors.RESET}")
                if params:
                    print(f"{Colors.BLUE}     Params: {params}{Colors.RESET}")
            
            response = requests.get(url, params=params, timeout=timeout)
            
            if self.config.get('test_execution', {}).get('verbose', True):
                print(f"{Colors.BLUE}  ← Status: {response.status_code}{Colors.RESET}")
                print(f"{Colors.BLUE}  ← Response: {response.text[:100]}...{Colors.RESET}" 
                      if len(response.text) > 100 else 
                      f"{Colors.BLUE}  ← Response: {response.text}{Colors.RESET}")
            
            return response.status_code, response.text
            
        except requests.exceptions.Timeout:
            return None, "Timeout"
        except Exception as e:
            return None, f"Error: {str(e)}"
    
    def send_post_request(self, endpoint: str, data: Dict) -> tuple:
        """Odoslanie POST requestu"""
        try:
            url = f"{self.base_url}{endpoint}"
            timeout = self.config['http'].get('timeout', 5)
            
            if self.config.get('test_execution', {}).get('verbose', True):
                print(f"{Colors.BLUE}  → POST {endpoint}{Colors.RESET}")
                print(f"{Colors.BLUE}     Data: {json.dumps(data)}{Colors.RESET}")
            
            response = requests.post(url, 
                                    json=data, 
                                    headers={'Content-Type': 'application/json'},
                                    timeout=timeout)
            
            if self.config.get('test_execution', {}).get('verbose', True):
                print(f"{Colors.BLUE}  ← Status: {response.status_code}{Colors.RESET}")
                print(f"{Colors.BLUE}  ← Response: {response.text[:100]}...{Colors.RESET}" 
                      if len(response.text) > 100 else 
                      f"{Colors.BLUE}  ← Response: {response.text}{Colors.RESET}")
            
            return response.status_code, response.text
            
        except requests.exceptions.Timeout:
            return None, "Timeout"
        except Exception as e:
            return None, f"Error: {str(e)}"
    
    def validate_response(self, status_code: int, response: str, 
                         expected: Any, expected_type: str) -> tuple[bool, str]:
        """Validácia odpovede"""
        if status_code is None:
            return False, response  # response obsahuje error message
        
        try:
            if expected_type == "status_code":
                # Kontrola HTTP status kódu
                passed = status_code == expected
                error = None if passed else f"Očakávaný status: {expected}, Dostané: {status_code}"
                
            elif expected_type == "json_key":
                # Kontrola kľúča v JSON odpovedi
                try:
                    json_response = json.loads(response)
                    passed = expected in json_response
                    error = None if passed else f"JSON neobsahuje kľúč: '{expected}'"
                except json.JSONDecodeError:
                    passed = False
                    error = "Odpoveď nie je platný JSON"
                    
            elif expected_type == "json_value":
                # Kontrola hodnoty v JSON odpovedi
                try:
                    json_response = json.loads(response)
                    key = expected.get('key')
                    value = expected.get('value')
                    
                    # Navigácia cez vnorené JSON (napr. "result.version")
                    keys = key.split('.')
                    current = json_response
                    for k in keys:
                        current = current.get(k)
                        if current is None:
                            passed = False
                            error = f"Kľúč '{key}' neexistuje"
                            break
                    else:
                        passed = current == value
                        error = None if passed else f"Očakávaná hodnota '{value}', Dostané: '{current}'"
                        
                except json.JSONDecodeError:
                    passed = False
                    error = "Odpoveď nie je platný JSON"
                    
            elif expected_type == "contains":
                # Obsahuje reťazec
                passed = expected in response
                error = None if passed else f"Odpoveď neobsahuje: '{expected}'"
                
            elif expected_type == "exact":
                # Presná zhoda
                passed = response == expected
                error = None if passed else f"Očakávané: '{expected}', Dostané: '{response}'"
                
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
        
        # Odoslanie requestu
        method = test.get('method', 'GET').upper()
        endpoint = test.get('endpoint', '/')
        request_params = None
        
        if method == 'GET':
            # Pre GET /api musíme JSON dať do query parametra "request"
            if endpoint == '/api' and 'data' in test:
                params = {'request': json.dumps(test['data'])}
                request_params = params
                status_code, response = self.send_get_request(endpoint, params)
            else:
                params = test.get('params', {})
                request_params = params if params else None
                status_code, response = self.send_get_request(endpoint, params)
        elif method == 'POST':
            data = test.get('data', {})
            status_code, response = self.send_post_request(endpoint, data)
        else:
            status_code = None
            response = f"Nepodporovaná metóda: {method}"
        
        # Validácia odpovede
        expected = test.get('expected', '')
        expected_type = test.get('expected_type', 'status_code')
        
        passed, error = self.validate_response(status_code, response, expected, expected_type)
        
        duration = time.time() - start_time
        
        # Výsledok
        result = TestResult(
            test_id=test_id,
            name=name,
            passed=passed,
            request={
                'method': method, 
                'endpoint': endpoint, 
                'data': test.get('data', {}),
                'params': request_params
            },
            expected=expected,
            actual=response,
            duration=duration,
            error=error,
            status_code=status_code
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
        print(f"{Colors.BOLD}Spúšťam {len(tests)} HTTP API testov...{Colors.RESET}")
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
        print(f"{Colors.BOLD}SUMÁR HTTP API TESTOV{Colors.RESET}")
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
    
    def save_html_report(self, timestamp: str):
        """Uloženie HTML reportu"""
        filename = f"http_test_report_{timestamp}.html"
        
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed
        success_rate = (passed / total * 100) if total > 0 else 0
        
        html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>HTTP API Test Report - {timestamp}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        h1 {{ color: #333; border-bottom: 3px solid #2196F3; padding-bottom: 10px; }}
        .summary {{ background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        .summary-grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }}
        .summary-item {{ text-align: center; }}
        .summary-item .value {{ font-size: 32px; font-weight: bold; }}
        .summary-item .label {{ color: #666; font-size: 14px; }}
        .passed {{ color: #4CAF50; }}
        .failed {{ color: #f44336; }}
        table {{ width: 100%; border-collapse: collapse; margin-top: 20px; }}
        th {{ background: #2196F3; color: white; padding: 12px; text-align: left; }}
        td {{ padding: 10px; border-bottom: 1px solid #ddd; }}
        tr:hover {{ background: #f5f5f5; }}
        .test-passed {{ background: #e8f5e9; }}
        .test-failed {{ background: #ffebee; }}
        .method {{ font-weight: bold; padding: 3px 8px; border-radius: 3px; font-size: 11px; }}
        .method-get {{ background: #4CAF50; color: white; }}
        .method-post {{ background: #FF9800; color: white; }}
        .endpoint {{ font-family: monospace; color: #1976D2; }}
        .error {{ color: #f44336; font-size: 12px; margin-top: 5px; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>HTTP API Test Report</h1>
        <p>Dátum: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        <p>Base URL: {self.base_url}</p>
        
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
                    <th>Request</th>
                    <th>Status</th>
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
            
            method = result.request['method']
            method_class = f"method-{method.lower()}"
            endpoint = result.request['endpoint']
            
            html += f"""
                <tr class="{status_class}">
                    <td><strong>{result.test_id}</strong></td>
                    <td>{result.name}</td>
                    <td>
                        <span class="method {method_class}">{method}</span>
                        <span class="endpoint">{endpoint}</span>
                    </td>
                    <td>{result.status_code if result.status_code else 'N/A'}</td>
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
    
    def save_report(self):
        """Uloženie reportu do súboru"""
        if not self.save_individual_report:
            return
        
        test_execution = self.config.get('test_execution', {})
        
        if not test_execution.get('save_report', True):
            return
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        self.save_html_report(timestamp)
    
    def get_test_data(self) -> dict:
        """Vráti dáta testov pre unified report"""
        tests = []
        for result in self.results:
            # Zostaviť úplnú URL s parametrami
            method = result.request.get('method', 'N/A')
            endpoint = result.request.get('endpoint', '')
            params = result.request.get('params')
            data = result.request.get('data', {})
            
            # Formátuj request string
            if method == 'GET' and params:
                # GET s parametrami
                param_str = '&'.join([f"{k}={v}" for k, v in params.items()])
                full_url = f"{self.base_url}{endpoint}?{param_str}"
                request_str = f"GET {full_url}"
            elif method == 'POST' and data:
                # POST s dátami
                full_url = f"{self.base_url}{endpoint}"
                data_str = str(data)[:80] + '...' if len(str(data)) > 80 else str(data)
                request_str = f"POST {full_url} | Data: {data_str}"
            else:
                # Iné
                full_url = f"{self.base_url}{endpoint}"
                request_str = f"{method} {full_url}"
            
            test_dict = {
                'id': result.test_id,
                'name': result.name,
                'status': 'passed' if result.passed else ('skipped' if hasattr(result, 'skipped') and result.skipped else 'failed'),
                'duration': result.duration,
                'method': method,
                'url': result.request.get('endpoint', 'N/A'),
                'full_request': request_str,
                'status_code': result.status_code if result.status_code else 'N/A',
                'response': str(result.actual)[:200] if result.actual else '',  # Limit na 200 znakov
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
        print(f"{Colors.BOLD}ESP32 HTTP API Test Runner{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}\n")
        
        # Načítanie konfigurácie
        if not self.load_config():
            return 1
        
        # Kontrola pripojenia
        if not self.check_connection():
            print(f"\n{Colors.RED}Nemôžem sa pripojiť na ESP32. Skontroluj:{Colors.RESET}")
            print(f"  - Či je ESP32 zapnuté")
            print(f"  - IP adresu a port v {self.config_file}")
            print(f"  - Sieťové pripojenie")
            return 1
        
        # Spustenie testov
        self.run_all_tests()
        
        # Výpis súhrnu
        self.print_summary()
        
        # Uloženie reportu
        self.save_report()
        
        # Návratový kód podľa výsledkov
        failed_count = sum(1 for r in self.results if not r.passed)
        return 0 if failed_count == 0 else 1

def main():
    """Entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='HTTP API Test Runner pre ESP32')
    parser.add_argument('-c', '--config', default='http_test_config.yaml',
                       help='Cesta ku konfiguračnému súboru (default: http_test_config.yaml)')
    parser.add_argument('-v', '--version', action='version', version='1.0.0')
    
    args = parser.parse_args()
    
    runner = HTTPTestRunner(args.config)
    exit_code = runner.run()
    
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
