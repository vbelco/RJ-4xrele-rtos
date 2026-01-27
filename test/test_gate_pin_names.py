#!/usr/bin/env python3
"""
Test runner pre textové názvy pinov (GATE1, GATE2, GATE3, GATE4)
Spustí sa pomocou: python3 test_gate_pin_names.py
"""

import sys
import os

# Pridaj adresár s testami do sys.path
sys.path.insert(0, os.path.dirname(__file__))

from mqtt_test_runner import MQTTTestRunner, Colors

def main():
    """Hlavná funkcia"""
    print(f"\n{Colors.BOLD}{'='*70}{Colors.RESET}")
    print(f"{Colors.BOLD}  Test: Textové názvy pinov (GATE1-GATE4){Colors.RESET}")
    print(f"{Colors.BOLD}{'='*70}{Colors.RESET}\n")
    
    # Vytvor test runner s konfiguráciou pre gate pin names
    runner = MQTTTestRunner(config_file="gate_pin_names_tests.yaml")
    
    # Načítaj konfiguráciu
    if not runner.load_config():
        print(f"{Colors.RED}Chyba pri načítaní konfigurácie{Colors.RESET}")
        return 1
    
    # Pripoj sa na MQTT
    if not runner.connect_mqtt():
        print(f"{Colors.RED}Chyba pri pripojení na MQTT broker{Colors.RESET}")
        return 1
    
    # Spusti testy
    print(f"\n{Colors.BLUE}Spúšťam testy...{Colors.RESET}\n")
    runner.run_tests()
    
    # Odpoj sa
    runner.disconnect_mqtt()
    
    # Zobraz výsledky
    print(f"\n{Colors.BOLD}{'='*70}{Colors.RESET}")
    print(f"{Colors.BOLD}  VÝSLEDKY TESTOV{Colors.RESET}")
    print(f"{Colors.BOLD}{'='*70}{Colors.RESET}\n")
    
    runner.print_summary()
    runner.print_detailed_results()
    
    # Vygeneruj report
    report_file = runner.generate_report()
    if report_file:
        print(f"\n{Colors.GREEN}Report uložený: {report_file}{Colors.RESET}")
    
    # Vráť exit code podľa výsledkov
    return 0 if all(r.passed for r in runner.results) else 1

if __name__ == "__main__":
    try:
        exit_code = main()
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}Test prerušený používateľom{Colors.RESET}")
        sys.exit(1)
    except Exception as e:
        print(f"\n{Colors.RED}Neočakávaná chyba: {e}{Colors.RESET}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
