#!/usr/bin/env python3
"""
Stress Test Runner - pre rýchle sekvenčné a paralelné testy MQTT + HTTP
"""

import paho.mqtt.client as mqtt
import requests
import json
import yaml
import time
import sys
import os
import threading
import ssl
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any, Optional
from concurrent.futures import ThreadPoolExecutor, as_completed

class Colors:
    """ANSI farby pre terminál"""
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

class StressTestResult:
    """Výsledok stress testu"""
    def __init__(self, test_id: str, name: str, passed: bool, 
                 expected_count: int, actual_count: int,
                 duration: float, error: str = None):
        self.test_id = test_id
        self.name = name
        self.passed = passed
        self.expected_count = expected_count
        self.actual_count = actual_count
        self.duration = duration
        self.error = error
        self.timestamp = datetime.now()

class StressTestRunner:
    """Hlavná trieda pre stress testy"""
    
    def __init__(self, config_file: str = "stress_test_config.yaml"):
        self.config_file = config_file
        self.config = None
        self.mqtt_client = None
        self.mqtt_messages = []
        self.mqtt_lock = threading.Lock()
        self.results: List[StressTestResult] = []
        self.connected = False
        
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
                    if value.lower() == 'true':
                        value = True
                    elif value.lower() == 'false':
                        value = False
                    elif value.isdigit():
                        value = int(value)
                    env_vars[key] = value
        return env_vars
        
    def load_config(self):
        """Načítanie konfiguračného súboru"""
        try:
            with open(self.config_file, 'r', encoding='utf-8') as f:
                self.config = yaml.safe_load(f)
            
            # Načítaj .env súbor
            env_vars = self.load_env()
            
            # Vytvor mqtt sekciu z .env hodnôt
            if env_vars:
                self.config['mqtt'] = {
                    'broker': env_vars.get('MQTT_BROKER'),
                    'port': env_vars.get('MQTT_PORT'),
                    'username': env_vars.get('MQTT_USERNAME'),
                    'password': env_vars.get('MQTT_PASSWORD'),
                    'use_tls': env_vars.get('MQTT_USE_TLS'),
                    'ca_cert': env_vars.get('MQTT_CA_CERT'),
                    'topic_send': env_vars.get('MQTT_TOPIC_SEND'),
                    'topic_receive': env_vars.get('MQTT_TOPIC_RECEIVE'),
                    'timeout': env_vars.get('MQTT_TIMEOUT', 10)
                }
                
                # Parsuj HTTP_BASE_URL na device_ip a device_port
                http_base = env_vars.get('HTTP_BASE_URL', 'http://192.168.1.189:9090')
                # Odstráň http:// alebo https://
                http_base = http_base.replace('http://', '').replace('https://', '')
                if ':' in http_base:
                    ip, port = http_base.split(':')
                    self.config['device_ip'] = ip
                    self.config['device_port'] = int(port)
                else:
                    self.config['device_ip'] = http_base
                    self.config['device_port'] = 80
                
            print(f"{Colors.GREEN}✓{Colors.RESET} Konfigurácia načítaná: {self.config_file}")
            return True
        except Exception as e:
            print(f"{Colors.RED}✗{Colors.RESET} Chyba pri načítaní konfigurácie: {e}")
            return False
    
    def on_connect(self, client, userdata, flags, rc):
        """MQTT callback - pripojenie"""
        if rc == 0:
            self.connected = True
            topic = self.config['mqtt']['topic_receive']
            client.subscribe(topic)
            print(f"{Colors.GREEN}✓{Colors.RESET} MQTT pripojené, subscribe: {topic}")
        else:
            print(f"{Colors.RED}✗{Colors.RESET} MQTT pripojenie zlyhalo: {rc}")
    
    def on_message(self, client, userdata, msg):
        """MQTT callback - prijatie správy"""
        try:
            payload = msg.payload.decode('utf-8')
            with self.mqtt_lock:
                self.mqtt_messages.append({
                    'topic': msg.topic,
                    'payload': payload,
                    'timestamp': time.time()
                })
        except Exception as e:
            print(f"{Colors.YELLOW}⚠{Colors.RESET} Chyba pri spracovaní MQTT správy: {e}")
    
    def setup_mqtt(self):
        """Nastavenie MQTT klienta"""
        try:
            mqtt_config = self.config['mqtt']
            self.mqtt_client = mqtt.Client()
            self.mqtt_client.on_connect = self.on_connect
            self.mqtt_client.on_message = self.on_message
            
            if mqtt_config.get('username') and mqtt_config.get('password'):
                self.mqtt_client.username_pw_set(
                    mqtt_config['username'], 
                    mqtt_config['password']
                )
            
            if mqtt_config.get('use_tls'):
                ca_cert = mqtt_config.get('ca_cert', 'broker.crt')
                self.mqtt_client.tls_set(ca_cert, tls_version=ssl.PROTOCOL_TLSv1_2)
            
            self.mqtt_client.connect(
                mqtt_config['broker'],
                mqtt_config['port'],
                60
            )
            
            self.mqtt_client.loop_start()
            
            # Počkaj na pripojenie
            timeout = 5
            start = time.time()
            while not self.connected and (time.time() - start) < timeout:
                time.sleep(0.1)
            
            return self.connected
        except Exception as e:
            print(f"{Colors.RED}✗{Colors.RESET} MQTT setup error: {e}")
            return False
    
    def send_mqtt_command(self, command: Dict) -> bool:
        """Odoslanie MQTT príkazu"""
        try:
            topic = self.config['mqtt']['topic_send']
            payload = json.dumps(command)
            self.mqtt_client.publish(topic, payload)
            return True
        except Exception as e:
            print(f"{Colors.RED}✗{Colors.RESET} MQTT send error: {e}")
            return False
    
    def send_http_request(self, endpoint: str, method: str = "GET") -> Optional[Dict]:
        """Odoslanie HTTP požiadavky"""
        try:
            url = f"http://{self.config['device_ip']}:{self.config['device_port']}{endpoint}"
            
            if method == "GET":
                response = requests.get(url, timeout=5)
            elif method == "POST":
                response = requests.post(url, timeout=5)
            else:
                return None
            
            if response.status_code == 200:
                return {'status': 'success', 'data': response.text}
            else:
                return {'status': 'error', 'code': response.status_code}
        except Exception as e:
            return {'status': 'error', 'message': str(e)}
    
    def run_rapid_sequential_test(self, test: Dict) -> StressTestResult:
        """Spustenie rýchleho sekvenčného testu"""
        test_id = test['id']
        name = test['name']
        protocol = test.get('protocol', 'mqtt')
        
        print(f"\n{Colors.BLUE}▶{Colors.RESET} {test_id}: {name}")
        
        start_time = time.time()
        success_count = 0
        
        # Clear MQTT messages buffer
        with self.mqtt_lock:
            self.mqtt_messages.clear()
        
        try:
            if protocol == 'mqtt':
                # MQTT rapid test
                commands = test.get('commands', [test.get('command')])
                repeat = test.get('repeat', 1)
                count = test.get('count', len(commands) * repeat)
                delay_ms = test.get('delay_between_ms', 50)
                
                sent_count = 0
                for _ in range(repeat):
                    for cmd in commands:
                        if self.send_mqtt_command(cmd):
                            sent_count += 1
                        time.sleep(delay_ms / 1000.0)
                
                # Počkaj na odpovede
                max_duration = test.get('max_duration_ms', 5000) / 1000.0
                time.sleep(min(max_duration, 2.0))
                
                with self.mqtt_lock:
                    success_count = len(self.mqtt_messages)
                
                expected = test.get('expected_response_count', count)
                
            elif protocol == 'http':
                # HTTP rapid test
                endpoints = test.get('endpoints', [test.get('endpoint')])
                repeat = test.get('repeat', 1)
                count = test.get('count', len(endpoints) * repeat)
                delay_ms = test.get('delay_between_ms', 50)
                method = test.get('method', 'GET')
                
                for _ in range(repeat):
                    for endpoint in endpoints:
                        result = self.send_http_request(endpoint, method)
                        if result and result.get('status') == 'success':
                            success_count += 1
                        time.sleep(delay_ms / 1000.0)
                
                expected = test.get('expected_success_count', count)
            
            duration = time.time() - start_time
            passed = success_count >= expected
            
            if test.get('allow_partial_success') and success_count > 0:
                passed = True
            
            result = StressTestResult(
                test_id, name, passed,
                expected, success_count, duration
            )
            
            if passed:
                print(f"{Colors.GREEN}✓ PASS{Colors.RESET} - {success_count}/{expected} responses in {duration:.2f}s")
            else:
                print(f"{Colors.RED}✗ FAIL{Colors.RESET} - {success_count}/{expected} responses")
            
            return result
            
        except Exception as e:
            duration = time.time() - start_time
            result = StressTestResult(
                test_id, name, False, 0, success_count, duration, str(e)
            )
            print(f"{Colors.RED}✗ ERROR{Colors.RESET} - {e}")
            return result
    
    def run_parallel_test(self, test: Dict) -> StressTestResult:
        """Spustenie paralelného MQTT + HTTP testu"""
        test_id = test['id']
        name = test['name']
        
        print(f"\n{Colors.BLUE}▶{Colors.RESET} {test_id}: {name}")
        
        start_time = time.time()
        mqtt_success = 0
        http_success = 0
        
        # Clear MQTT messages buffer
        with self.mqtt_lock:
            self.mqtt_messages.clear()
        
        try:
            mqtt_commands = test.get('mqtt_commands', [test.get('mqtt_command')])
            http_endpoints = test.get('http_endpoints', [test.get('http_endpoint')])
            http_method = test.get('http_method', 'GET')
            repeat = test.get('repeat', 1)
            simultaneous = test.get('simultaneous_count', 1)
            delay_ms = test.get('delay_between_rounds_ms', 200)
            
            # Funkcia pre paralelné volanie
            def send_parallel_round():
                mqtt_sent = 0
                http_sent = 0
                
                with ThreadPoolExecutor(max_workers=20) as executor:
                    futures = []
                    
                    # MQTT príkazy
                    for cmd in mqtt_commands:
                        future = executor.submit(self.send_mqtt_command, cmd)
                        futures.append(('mqtt', future))
                    
                    # HTTP požiadavky
                    for endpoint in http_endpoints:
                        future = executor.submit(self.send_http_request, endpoint, http_method)
                        futures.append(('http', future))
                    
                    # Čakaj na dokončenie
                    for proto, future in futures:
                        try:
                            result = future.result(timeout=5)
                            if proto == 'mqtt' and result:
                                mqtt_sent += 1
                            elif proto == 'http' and result and result.get('status') == 'success':
                                http_sent += 1
                        except:
                            pass
                
                return mqtt_sent, http_sent
            
            # Opakuj paralelné volania
            for i in range(repeat if repeat > 1 else simultaneous):
                m, h = send_parallel_round()
                mqtt_success += m
                http_success += h
                if i < (repeat if repeat > 1 else simultaneous) - 1:
                    time.sleep(delay_ms / 1000.0)
            
            # Počkaj na MQTT odpovede
            max_duration = test.get('max_duration_ms', 5000) / 1000.0
            time.sleep(min(max_duration / 2, 1.0))
            
            with self.mqtt_lock:
                mqtt_received = len(self.mqtt_messages)
            
            duration = time.time() - start_time
            
            expected_mqtt = test.get('expected_mqtt_responses', len(mqtt_commands) * repeat)
            expected_http = test.get('expected_http_success', len(http_endpoints) * repeat)
            
            passed = (mqtt_received >= expected_mqtt and http_success >= expected_http)
            
            if test.get('allow_partial_success') and (mqtt_received > 0 or http_success > 0):
                passed = True
            
            result = StressTestResult(
                test_id, name, passed,
                expected_mqtt + expected_http,
                mqtt_received + http_success,
                duration
            )
            
            if passed:
                print(f"{Colors.GREEN}✓ PASS{Colors.RESET} - MQTT: {mqtt_received}/{expected_mqtt}, HTTP: {http_success}/{expected_http} in {duration:.2f}s")
            else:
                print(f"{Colors.RED}✗ FAIL{Colors.RESET} - MQTT: {mqtt_received}/{expected_mqtt}, HTTP: {http_success}/{expected_http}")
            
            return result
            
        except Exception as e:
            duration = time.time() - start_time
            result = StressTestResult(
                test_id, name, False, 0, 0, duration, str(e)
            )
            print(f"{Colors.RED}✗ ERROR{Colors.RESET} - {e}")
            return result
    
    def run_all_tests(self):
        """Spustenie všetkých testov"""
        print(f"\n{Colors.BOLD}=== ESP32 STRESS TESTY ==={Colors.RESET}\n")
        
        if not self.load_config():
            return False
        
        if not self.setup_mqtt():
            print(f"{Colors.RED}✗{Colors.RESET} MQTT pripojenie zlyhalo")
            return False
        
        total_tests = 0
        passed_tests = 0
        
        # Rapid Sequential Tests
        if 'rapid_sequential_tests' in self.config:
            print(f"\n{Colors.BOLD}--- RAPID SEQUENTIAL TESTS ---{Colors.RESET}")
            for test in self.config['rapid_sequential_tests']:
                if test.get('enabled', True):
                    result = self.run_rapid_sequential_test(test)
                    self.results.append(result)
                    total_tests += 1
                    if result.passed:
                        passed_tests += 1
        
        # Parallel Tests
        if 'parallel_tests' in self.config:
            print(f"\n{Colors.BOLD}--- PARALLEL TESTS (MQTT + HTTP) ---{Colors.RESET}")
            for test in self.config['parallel_tests']:
                if test.get('enabled', True):
                    result = self.run_parallel_test(test)
                    self.results.append(result)
                    total_tests += 1
                    if result.passed:
                        passed_tests += 1
        
        # Queue Overflow Tests
        if 'queue_overflow_tests' in self.config:
            print(f"\n{Colors.BOLD}--- QUEUE OVERFLOW TESTS ---{Colors.RESET}")
            for test in self.config['queue_overflow_tests']:
                if test.get('enabled', True):
                    # Rozlíš typ testu podľa protokolu
                    if 'protocol' in test:
                        result = self.run_rapid_sequential_test(test)
                    else:
                        result = self.run_parallel_test(test)
                    self.results.append(result)
                    total_tests += 1
                    if result.passed:
                        passed_tests += 1
        
        # Cleanup
        if self.mqtt_client:
            self.mqtt_client.loop_stop()
            self.mqtt_client.disconnect()
        
        # Summary
        print(f"\n{Colors.BOLD}=== SUMMARY ==={Colors.RESET}")
        print(f"Total: {total_tests}, Passed: {passed_tests}, Failed: {total_tests - passed_tests}")
        
        if passed_tests == total_tests:
            print(f"{Colors.GREEN}✓ ALL TESTS PASSED{Colors.RESET}")
        else:
            print(f"{Colors.RED}✗ SOME TESTS FAILED{Colors.RESET}")
        
        return passed_tests == total_tests

def main():
    """Main entry point"""
    config_file = sys.argv[1] if len(sys.argv) > 1 else "stress_test_config.yaml"
    
    runner = StressTestRunner(config_file)
    success = runner.run_all_tests()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
