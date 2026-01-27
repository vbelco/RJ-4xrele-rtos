#!/usr/bin/env python3
"""
MQTT Timing Test Runner - komplexné časové scenáre
Podporuje multi-step testy s meraním časovania a JSON path validáciu
"""

import time
import json
import yaml
import sys
from datetime import datetime
from mqtt_test_runner import MQTTTestRunner, Colors, TestResult
from typing import Dict, List, Any

class TimingTestRunner(MQTTTestRunner):
    """Rozšírený test runner pre časové scenáre"""
    
    def validate_json_path(self, response: str, json_path: str, expected_value: str) -> tuple:
        """Validácia hodnoty v JSON ceste (napr. gate_status.GATE1)"""
        try:
            json_response = json.loads(response)
            
            # Rozdeľ cestu (napr. "gate_status.GATE1" -> ["gate_status", "GATE1"])
            path_parts = json_path.split('.')
            
            # Prechádzaj JSON stromom
            current = json_response
            for part in path_parts:
                if isinstance(current, dict) and part in current:
                    current = current[part]
                else:
                    return False, f"JSON cesta '{json_path}' neexistuje v odpovedi"
            
            # Porovnaj hodnotu
            if str(current) == str(expected_value):
                return True, None
            else:
                return False, f"Hodnota na '{json_path}' je '{current}', očakávané '{expected_value}'"
                
        except json.JSONDecodeError:
            return False, "Odpoveď nie je platný JSON"
        except Exception as e:
            return False, f"Chyba validácie JSON path: {str(e)}"
    
    def run_step(self, step: Dict, step_num: int, total_steps: int) -> tuple:
        """Spustenie jedného kroku v multi-step teste"""
        command = step.get('command')
        expected = step.get('expected_response', {})
        wait_after = step.get('wait_after', 0)
        note = step.get('note', '')
        
        print(f"{Colors.BLUE}  Krok {step_num}/{total_steps}:{Colors.RESET} {note if note else json.dumps(command)}")
        
        # Odoslanie príkazu
        response = self.send_command(command)
        
        # Validácia odpovede ak je špecifikovaná
        if expected:
            exp_type = expected.get('type', 'exact')
            
            if exp_type == 'json_path':
                # JSON path validácia
                json_path = expected.get('path')
                exp_value = expected.get('value')
                passed, error = self.validate_json_path(response, json_path, exp_value)
            else:
                # Štandardná validácia
                exp_value = expected.get('value', '')
                passed, error = self.validate_response(response, exp_value, exp_type)
            
            if not passed:
                print(f"{Colors.RED}    ✗ {error}{Colors.RESET}")
                return False, error
            else:
                print(f"{Colors.GREEN}    ✓ OK{Colors.RESET}")
        
        # Čakanie po kroku
        if wait_after > 0:
            print(f"{Colors.YELLOW}    ⏱ Čakám {wait_after}s...{Colors.RESET}")
            time.sleep(wait_after)
        
        return True, None
    
    def run_multistep_test(self, test: Dict) -> TestResult:
        """Spustenie multi-step testu"""
        test_id = test.get('id', 'unknown')
        name = test.get('name', 'Unnamed test')
        steps = test.get('steps', [])
        
        print(f"\n{Colors.BOLD}[{test_id}] {name}{Colors.RESET}")
        print(f"{Colors.BLUE}  Multi-step test s {len(steps)} krokmi{Colors.RESET}")
        
        start_time = time.time()
        
        overall_passed = True
        error_message = None
        
        # Spusti všetky kroky
        for i, step in enumerate(steps, 1):
            passed, error = self.run_step(step, i, len(steps))
            if not passed:
                overall_passed = False
                error_message = f"Krok {i} zlyhal: {error}"
                break
        
        duration = time.time() - start_time
        
        # Vytvor detailný command text s krokmi
        steps_details = []
        for i, step in enumerate(steps, 1):
            cmd = step.get('command', {})
            wait = step.get('wait_after', 0)
            note = step.get('note', '')
            
            cmd_str = json.dumps(cmd, ensure_ascii=False)
            step_text = f"{i}. {cmd_str}"
            if wait > 0:
                step_text += f" ⏱ {wait}s"
            if note:
                step_text += f" ({note})"
            steps_details.append(step_text)
        
        command_text = f"Multi-step ({len(steps)} krokov):\n" + "\n".join(steps_details)
        
        # Výsledok
        result = TestResult(
            test_id=test_id,
            name=name,
            passed=overall_passed,
            command=command_text,
            expected="Všetky kroky úspešné",
            actual=f"{'Úspech' if overall_passed else error_message}",
            duration=duration,
            error=error_message
        )
        
        # Výpis výsledku
        if overall_passed:
            print(f"{Colors.GREEN}  ✓ PASSED{Colors.RESET} ({duration:.2f}s)")
        else:
            print(f"{Colors.RED}  ✗ FAILED{Colors.RESET} ({duration:.2f}s)")
            if error_message:
                print(f"{Colors.RED}    {error_message}{Colors.RESET}")
        
        return result
    
    def run_all_tests(self) -> bool:
        """Spustenie všetkých testov (podporuje single a multi-step)"""
        tests = self.config.get('tests', [])
        
        if not tests:
            print(f"{Colors.YELLOW}⚠ Žiadne testy nenájdené v konfigurácii{Colors.RESET}")
            return False
        
        print(f"\n{Colors.BOLD}{'='*60}{Colors.RESET}")
        print(f"{Colors.BOLD}Spúšťam {len(tests)} testov...{Colors.RESET}")
        print(f"{Colors.BOLD}{'='*60}{Colors.RESET}")
        
        for test in tests:
            # Detekuj či ide o multi-step test
            if 'steps' in test:
                result = self.run_multistep_test(test)
            else:
                result = self.run_test(test)
            
            self.results.append(result)
            
            # Delay medzi testami
            test_execution = self.config.get('test_execution', {})
            delay = test_execution.get('delay_between_tests', 0.5)
            time.sleep(delay)
        
        return True

def main():
    """Entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='MQTT Timing Test Runner pre ESP32')
    parser.add_argument('-c', '--config', default='gate_timing_tests.yaml',
                       help='Cesta ku konfiguračnému súboru (default: gate_timing_tests.yaml)')
    parser.add_argument('-v', '--version', action='version', version='1.0.0')
    
    args = parser.parse_args()
    
    runner = TimingTestRunner(args.config)
    exit_code = runner.run()
    
    sys.exit(exit_code)

if __name__ == '__main__':
    main()
