#!/usr/bin/env python3
"""
ESP32 STATUS Integrity Test Runner
Rozšírenie timing_test_runner.py o custom validátory pre kontrolu integrity STATUS hodnôt
"""

import sys
import json
import time
from timing_test_runner import TimingTestRunner
from mqtt_test_runner import Colors

class StatusIntegrityTestRunner(TimingTestRunner):
    """Test runner s custom validátormi pre STATUS integrity"""
    
    def __init__(self, config_file, save_individual_report=True):
        super().__init__(config_file, save_individual_report)
        self.saved_values = {}  # Úložisko pre medzivýsledky
    
    def validate_custom(self, response, validator_name, gate=None):
        """
        Vykonanie custom validácie podľa názvu validátora
        
        Args:
            response: JSON odpoveď zo STATUS
            validator_name: Názov validátora
            gate: Číslo/názov brány (ak je potrebné)
        
        Returns:
            (bool, str): (úspech, chybová správa)
        """
        try:
            status = json.loads(response)
        except json.JSONDecodeError:
            return False, "Neplatná JSON odpoveď"
        
        # Validátor: gate_off_time ≈ current_millis + duration
        if validator_name == "check_gate_off_time_calculation":
            return self._validate_gate_off_time_calculation(status, "GATE3", 5000)
        
        # Validátor: gate_off_time > current_millis
        elif validator_name == "check_gate_off_time_greater":
            return self._validate_gate_off_time_greater(status, "GATE4")
        
        # Validátor: Všetky brány majú správnu integritu
        elif validator_name == "check_all_gates_integrity":
            return self._validate_all_gates_integrity(status)
        
        # Validátor: Uloženie gate_off_time pre budúce porovnanie
        elif validator_name == "save_gate_off_time_1":
            return self._save_gate_off_time(status, "GATE1", "saved_1")
        
        # Validátor: Kontrola že nový gate_off_time > starý
        elif validator_name == "check_gate_off_time_updated":
            return self._check_gate_off_time_updated(status, "GATE1", "saved_1")
        
        else:
            return False, f"Neznámy validátor: {validator_name}"
    
    def _validate_gate_off_time_calculation(self, status, gate, duration_ms, tolerance=200):
        """
        Kontrola či gate_off_time ≈ current_millis + duration (s toleranciou)
        
        Poznámka: ESP32 môže mať malé oneskorenie medzi zapnutím a STATUS dotazom
        """
        current_millis = status.get('current_millis')
        gate_off_time = status.get('gate_off_time', {}).get(gate)
        
        if current_millis is None:
            return False, "Chýba current_millis v STATUS"
        
        if gate_off_time is None:
            return False, f"Chýba gate_off_time.{gate} v STATUS"
        
        # Vypočítaj očakávanú hodnotu (približne, lebo medzi príkazom a STATUS je delay)
        # gate_off_time by mal byť > current_millis (ešte beží)
        # a < current_millis + duration + tolerance (nie príliš dlho)
        
        if gate_off_time <= current_millis:
            return False, f"gate_off_time ({gate_off_time}) <= current_millis ({current_millis})"
        
        # Zostávajúci čas
        remaining = gate_off_time - current_millis
        
        # Očakávaný zostávajúci čas je duration_ms (mínus čas medzi zapnutím a dotazom)
        # Tolerancia: akceptujeme -500ms až +tolerance (kvôli network delay)
        if remaining < (duration_ms - 500) or remaining > (duration_ms + tolerance):
            return False, f"Zostávajúci čas {remaining}ms je mimo očakávania {duration_ms}ms (±{tolerance}ms, -500ms)"
        
        return True, f"OK: gate_off_time = {gate_off_time}, current_millis = {current_millis}, zostáva {remaining}ms"
    
    def _validate_gate_off_time_greater(self, status, gate):
        """
        Kontrola či gate_off_time > current_millis (časovač ešte beží)
        """
        current_millis = status.get('current_millis')
        gate_off_time = status.get('gate_off_time', {}).get(gate)
        
        if current_millis is None:
            return False, "Chýba current_millis v STATUS"
        
        if gate_off_time is None:
            return False, f"Chýba gate_off_time.{gate} v STATUS"
        
        if gate_off_time <= current_millis:
            return False, f"gate_off_time ({gate_off_time}) <= current_millis ({current_millis})"
        
        remaining = gate_off_time - current_millis
        return True, f"OK: gate_off_time > current_millis (zostáva {remaining}ms)"
    
    def _validate_all_gates_integrity(self, status):
        """
        Kontrola integrity všetkých brán:
        - GATE_DOWN → gate_off_time == -1
        - GATE_UP + endless → gate_off_time == 0
        - GATE_UP + timer → gate_off_time > current_millis
        """
        gate_status = status.get('gate_status', {})
        gate_off_time = status.get('gate_off_time', {})
        current_millis = status.get('current_millis')
        
        if not gate_status or not gate_off_time or current_millis is None:
            return False, "Chýbajúce polia v STATUS"
        
        errors = []
        
        for gate in ['GATE1', 'GATE2', 'GATE3', 'GATE4']:
            g_status = gate_status.get(gate)
            g_off_time = gate_off_time.get(gate)
            
            if g_status is None or g_off_time is None:
                errors.append(f"{gate}: chýbajúce údaje")
                continue
            
            # GATE_DOWN → musí mať -1
            if g_status == "GATE_DOWN":
                if g_off_time != -1:
                    errors.append(f"{gate}: GATE_DOWN ale gate_off_time={g_off_time} (očakávané -1)")
            
            # GATE_UP → musí mať buď 0 (endless) alebo > current_millis (timer)
            elif g_status == "GATE_UP":
                if g_off_time == -1:
                    errors.append(f"{gate}: GATE_UP ale gate_off_time=-1")
                elif g_off_time > 0 and g_off_time <= current_millis:
                    errors.append(f"{gate}: GATE_UP ale gate_off_time={g_off_time} <= current_millis={current_millis}")
        
        if errors:
            return False, "; ".join(errors)
        
        return True, "Všetky brány majú správnu integritu"
    
    def _save_gate_off_time(self, status, gate, key):
        """Uloženie gate_off_time pre budúce porovnanie"""
        gate_off_time = status.get('gate_off_time', {}).get(gate)
        
        if gate_off_time is None:
            return False, f"Chýba gate_off_time.{gate} v STATUS"
        
        self.saved_values[key] = gate_off_time
        return True, f"Uložené: {key} = {gate_off_time}"
    
    def _check_gate_off_time_updated(self, status, gate, saved_key):
        """Kontrola že nový gate_off_time > uložený"""
        gate_off_time = status.get('gate_off_time', {}).get(gate)
        saved_value = self.saved_values.get(saved_key)
        
        if gate_off_time is None:
            return False, f"Chýba gate_off_time.{gate} v STATUS"
        
        if saved_value is None:
            return False, f"Chýba uložená hodnota {saved_key}"
        
        if gate_off_time <= saved_value:
            return False, f"Nový gate_off_time ({gate_off_time}) <= starý ({saved_value})"
        
        diff = gate_off_time - saved_value
        return True, f"OK: gate_off_time sa aktualizoval ({saved_value} → {gate_off_time}, rozdiel {diff}ms)"
    
    def run_step(self, step, step_num, total_steps):
        """Override run_step pre pridanie custom validácie"""
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
            
            if exp_type == 'custom':
                # Custom validácia
                validator = expected.get('validator')
                if validator:
                    passed, message = self.validate_custom(response, validator)
                    if not passed:
                        print(f"{Colors.RED}    ✗ {message}{Colors.RESET}")
                        return False, message
                    else:
                        print(f"{Colors.GREEN}    ✓ {message}{Colors.RESET}")
                else:
                    return False, "Chýba validator pre custom validáciu"
            
            elif exp_type == 'json_path':
                # JSON path validácia
                json_path = expected.get('path')
                exp_value = expected.get('value')
                passed, error = self.validate_json_path(response, json_path, exp_value)
                if not passed:
                    print(f"{Colors.RED}    ✗ {error}{Colors.RESET}")
                    return False, error
                else:
                    print(f"{Colors.GREEN}    ✓ OK{Colors.RESET}")
            
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


def main():
    if len(sys.argv) < 2:
        print("Použitie: python3 status_integrity_test_runner.py <config_file>")
        sys.exit(1)
    
    config_file = sys.argv[1]
    runner = StatusIntegrityTestRunner(config_file)
    
    print("=" * 60)
    print("ESP32 STATUS Integrity Test Runner")
    print("=" * 60)
    print()
    
    # Použijeme run() namiesto run_all_tests() - automaticky načíta config
    exit_code = runner.run()
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
