#!/usr/bin/env python3
"""
Unified Test Runner - spustí MQTT aj HTTP API testy
"""

import sys
import os
from datetime import datetime

# Import test runnerov
from mqtt_test_runner import MQTTTestRunner, Colors
from http_test_runner import HTTPTestRunner
from timing_test_runner import TimingTestRunner
from status_integrity_test_runner import StatusIntegrityTestRunner

def create_unified_html_report(mqtt_results, gate_results, http_results, timing_results, status_timing_results=None, integrity_results=None, total_duration=0, filename="unified_test_report.html"):
    """Vytvorí jeden spoločný HTML report v pôvodnom formáte"""
    
    # Zbierka všetkých testov
    all_tests = []
    
    if mqtt_results:
        for test in mqtt_results.get('tests', []):
            test['source'] = 'MQTT'
            all_tests.append(test)
    
    if gate_results:
        for test in gate_results.get('tests', []):
            test['source'] = 'MQTT Gate Pins'
            all_tests.append(test)
    
    if timing_results:
        for test in timing_results.get('tests', []):
            test['source'] = 'MQTT Timing'
            all_tests.append(test)
    
    if status_timing_results:
        for test in status_timing_results.get('tests', []):
            test['source'] = 'MQTT Status'
            all_tests.append(test)
    
    if integrity_results:
        for test in integrity_results.get('tests', []):
            test['source'] = 'MQTT Integrity'
            all_tests.append(test)
    
    if http_results:
        for test in http_results.get('tests', []):
            test['source'] = 'HTTP'
            all_tests.append(test)
    
    # Štatistiky
    total_tests = len(all_tests)
    total_passed = sum(1 for t in all_tests if t['status'] == 'passed')
    total_failed = sum(1 for t in all_tests if t['status'] == 'failed')
    success_rate = (total_passed / total_tests * 100) if total_tests > 0 else 0
    
    html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Test Report - {datetime.now().strftime('%Y%m%d_%H%M%S')}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        .container {{ max-width: 1400px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
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
        .test-skipped {{ background: #fff9c4; }}
        .command {{ font-family: monospace; background: #f5f5f5; padding: 8px; border-radius: 3px; font-size: 11px; line-height: 1.6; display: block; }}
        .error {{ color: #f44336; font-size: 12px; margin-top: 5px; }}
        .source-badge {{ display: inline-block; padding: 3px 8px; border-radius: 3px; font-size: 11px; font-weight: bold; }}
        .source-mqtt {{ background: #2196F3; color: white; }}
        .source-http {{ background: #FF9800; color: white; }}
        .source-gate {{ background: #9C27B0; color: white; }}
        .source-timing {{ background: #00BCD4; color: white; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Test Report - Všetky testy</h1>
        <p>Dátum: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        
        <div class="summary">
            <div class="summary-grid">
                <div class="summary-item">
                    <div class="value">{total_tests}</div>
                    <div class="label">Celkovo testov</div>
                </div>
                <div class="summary-item passed">
                    <div class="value">{total_passed}</div>
                    <div class="label">Úspešných</div>
                </div>
                <div class="summary-item failed">
                    <div class="value">{total_failed}</div>
                    <div class="label">Neúspešných</div>
                </div>
                <div class="summary-item">
                    <div class="value">{success_rate:.1f}%</div>
                    <div class="label">Úspešnosť</div>
                </div>
                <div class="summary-item">
                    <div class="value">{total_duration:.1f}s</div>
                    <div class="label">Celkový čas</div>
                </div>
            </div>
        </div>
        
        <table>
            <thead>
                <tr>
                    <th>Typ</th>
                    <th>ID</th>
                    <th>Názov</th>
                    <th>Príkaz/Request</th>
                    <th>Výsledok</th>
                    <th>Čas</th>
                </tr>
            </thead>
            <tbody>
"""
    
    for test in all_tests:
        status_class = f"test-{test['status']}"
        status_text = {"passed": "✓ PASSED", "failed": "✗ FAILED", "skipped": "⊘ SKIPPED"}.get(test['status'], test['status'])
        status_color = test['status']
        
        # Badge pre typ testu
        source = test.get('source', 'Unknown')
        if source == 'MQTT':
            badge_class = 'source-mqtt'
        elif source == 'MQTT Gate Pins':
            badge_class = 'source-gate'
        elif source == 'MQTT Timing':
            badge_class = 'source-timing'
        elif source == 'MQTT Integrity':
            badge_class = 'source-timing'  # Použijeme rovnakú farbu ako timing
        elif source == 'HTTP':
            badge_class = 'source-http'
        else:
            badge_class = ''
        
        # Príkaz alebo request
        if 'full_request' in test:
            # HTTP test s úplnou URL
            cmd_text = test['full_request']
        elif 'command' in test:
            # MQTT test
            cmd_text = test['command']
            # Pre multi-step testy formatuj s line breaks
            if '\n' in cmd_text:
                cmd_text = cmd_text.replace('\n', '<br>')
        elif 'method' in test and 'url' in test:
            # Fallback
            cmd_text = f"{test['method']} {test['url']}"
        else:
            cmd_text = "N/A"
        
        html += f"""
                <tr class="{status_class}">
                    <td><span class="source-badge {badge_class}">{source}</span></td>
                    <td><strong>{test['id']}</strong></td>
                    <td>{test['name']}</td>
                    <td><code class="command" style="white-space: pre-wrap;">{cmd_text}</code></td>
                    <td class="{status_color}"><strong>{status_text}</strong>
                        {f'<div class="error">{test.get("error", "")}</div>' if test.get('error') else ''}
                    </td>
                    <td>{test['duration']:.2f}s</td>
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
    
    return filename

def print_header(text):
    """Vypíše header"""
    print(f"\n{Colors.BOLD}{'='*70}{Colors.RESET}")
    print(f"{Colors.BOLD}{text:^70}{Colors.RESET}")
    print(f"{Colors.BOLD}{'='*70}{Colors.RESET}\n")

def main():
    """Hlavná funkcia - spustenie všetkých testov"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Unified Test Runner pre ESP32')
    parser.add_argument('-m', '--mqtt-only', action='store_true',
                       help='Spustiť len MQTT testy')
    parser.add_argument('-H', '--http-only', action='store_true',
                       help='Spustiť len HTTP testy')
    parser.add_argument('--mqtt-config', default='test_config.yaml',
                       help='MQTT konfiguračný súbor')
    parser.add_argument('--http-config', default='http_test_config.yaml',
                       help='HTTP konfiguračný súbor')
    parser.add_argument('--skip-gate-pin-tests', action='store_true',
                       help='Preskočiť testy pre textové názvy pinov')
    parser.add_argument('--skip-timing-tests', action='store_true',
                       help='Preskočiť časové testy pre gate_ms príkazy')
    
    args = parser.parse_args()
    
    start_time = datetime.now()
    
    print_header("ESP32 UNIFIED TEST RUNNER")
    print(f"Dátum: {start_time.strftime('%Y-%m-%d %H:%M:%S')}\n")
    
    # Rozhodnutie ktoré testy spustiť
    run_mqtt = not args.http_only
    run_http = not args.mqtt_only
    
    results = {
        'mqtt': None,
        'mqtt_gate_pins': None,
        'mqtt_timing': None,
        'mqtt_status': None,
        'http': None
    }
    
    # Dáta pre unified report
    test_data = {
        'mqtt': None,
        'mqtt_gate_pins': None,
        'mqtt_timing': None,
        'mqtt_status': None,
        'http': None
    }
    
    # ========== MQTT TESTY ==========
    if run_mqtt:
        print_header("MQTT TESTY - Základné")
        
        if os.path.exists(args.mqtt_config):
            mqtt_runner = MQTTTestRunner(args.mqtt_config, save_individual_report=False)
            results['mqtt'] = mqtt_runner.run()
            test_data['mqtt'] = mqtt_runner.get_test_data()
        else:
            print(f"{Colors.RED}✗ MQTT config súbor nebol nájdený: {args.mqtt_config}{Colors.RESET}")
            results['mqtt'] = 1
    
    # ========== MQTT GATE PIN NAMES TESTY ==========
    if run_mqtt and not args.skip_gate_pin_tests:
        print_header("MQTT TESTY - Textové názvy pinov (GATE1-GATE4)")
        
        gate_pin_config = 'gate_pin_names_tests.yaml'
        if os.path.exists(gate_pin_config):
            gate_runner = MQTTTestRunner(gate_pin_config, save_individual_report=False)
            results['mqtt_gate_pins'] = gate_runner.run()
            test_data['mqtt_gate_pins'] = gate_runner.get_test_data()
        else:
            print(f"{Colors.YELLOW}⚠ Gate pin names config nebol nájdený: {gate_pin_config}{Colors.RESET}")
            print(f"{Colors.YELLOW}  Preskakujem testy pre textové názvy pinov{Colors.RESET}")
            results['mqtt_gate_pins'] = None
    
    # ========== MQTT TIMING TESTY ==========
    if run_mqtt and not args.skip_timing_tests:
        print_header("MQTT TESTY - Časovanie gate_ms príkazov")
        
        timing_config = 'gate_timing_tests.yaml'
        if os.path.exists(timing_config):
            timing_runner = TimingTestRunner(timing_config, save_individual_report=False)
            results['mqtt_timing'] = timing_runner.run()
            test_data['mqtt_timing'] = timing_runner.get_test_data()
        else:
            print(f"{Colors.YELLOW}⚠ Timing config nebol nájdený: {timing_config}{Colors.RESET}")
            print(f"{Colors.YELLOW}  Preskakujem časové testy{Colors.RESET}")
            results['mqtt_timing'] = None
    
    # ========== MQTT STATUS TIMING TESTY ==========
    if run_mqtt:
        print_header("MQTT TESTY - STATUS polia (current_millis, gate_off_time)")
        
        status_timing_config = 'test_status_timing.yaml'
        if os.path.exists(status_timing_config):
            status_runner = MQTTTestRunner(status_timing_config, save_individual_report=False)
            results['mqtt_status'] = status_runner.run()
            test_data['mqtt_status'] = status_runner.get_test_data()
        else:
            print(f"{Colors.YELLOW}⚠ Status timing config nebol nájdený: {status_timing_config}{Colors.RESET}")
            print(f"{Colors.YELLOW}  Preskakujem STATUS testy{Colors.RESET}")
            results['mqtt_status'] = None
    
    # ========== MQTT STATUS INTEGRITY TESTY ==========
    if run_mqtt:
        print_header("MQTT TESTY - STATUS integrita (gate_off_time validácia)")
        
        integrity_config = 'test_status_integrity.yaml'
        if os.path.exists(integrity_config):
            integrity_runner = StatusIntegrityTestRunner(integrity_config)
            results['mqtt_integrity'] = integrity_runner.run()
            test_data['mqtt_integrity'] = integrity_runner.get_test_data()
        else:
            print(f"{Colors.YELLOW}⚠ Status integrity config nebol nájdený: {integrity_config}{Colors.RESET}")
            print(f"{Colors.YELLOW}  Preskakujem integrity testy{Colors.RESET}")
            results['mqtt_integrity'] = None
            results['mqtt_status'] = None
    
    # ========== HTTP TESTY ==========
    if run_http:
        print_header("HTTP API TESTY")
        
        if os.path.exists(args.http_config):
            http_runner = HTTPTestRunner(args.http_config, save_individual_report=False)
            test_data.get('mqtt_integrity'),
            results['http'] = http_runner.run()
            test_data['http'] = http_runner.get_test_data()
        else:
            print(f"{Colors.RED}✗ HTTP config súbor nebol nájdený: {args.http_config}{Colors.RESET}")
            results['http'] = 1
    
    # ========== FINÁLNY SUMÁR ==========
    end_time = datetime.now()
    duration = (end_time - start_time).total_seconds()
    
    # Vytvor unified HTML report
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    report_filename = f"unified_test_report_{timestamp}.html"
    
    try:
        report_path = create_unified_html_report(
            test_data['mqtt'],
            test_data['mqtt_gate_pins'],
            test_data['http'],
            test_data['mqtt_timing'],
            test_data['mqtt_status'],
            test_data.get('mqtt_integrity'),
            duration,
            report_filename
        )
        print(f"\n{Colors.GREEN}✓ Unified HTML report uložený: {report_filename}{Colors.RESET}\n")
    except Exception as e:
        print(f"\n{Colors.YELLOW}⚠ Nepodarilo sa vytvoriť unified report: {e}{Colors.RESET}\n")
    
    print_header("FINÁLNY SUMÁR")
    
    if results['mqtt'] is not None:
        mqtt_status = f"{Colors.GREEN}✓ PASSED{Colors.RESET}" if results['mqtt'] == 0 else f"{Colors.RED}✗ FAILED{Colors.RESET}"
        print(f"MQTT Testy (základné):        {mqtt_status}")
    
    if results['mqtt_gate_pins'] is not None:
        gate_status = f"{Colors.GREEN}✓ PASSED{Colors.RESET}" if results['mqtt_gate_pins'] == 0 else f"{Colors.RED}✗ FAILED{Colors.RESET}"
        print(f"MQTT Testy (gate pin names):  {gate_status}")
    
    if results.get('mqtt_timing') is not None:
        timing_status = f"{Colors.GREEN}✓ PASSED{Colors.RESET}" if results['mqtt_timing'] == 0 else f"{Colors.RED}✗ FAILED{Colors.RESET}"
        print(f"MQTT Testy (časovanie):       {timing_status}")
    
    if results.get('mqtt_status') is not None:
        status_status = f"{Colors.GREEN}✓ PASSED{Colors.RESET}" if results['mqtt_status'] == 0 else f"{Colors.RED}✗ FAILED{Colors.RESET}"
        print(f"MQTT Testy (STATUS polia):    {status_status}")
    
    if results['http'] is not None:
        http_status = f"{Colors.GREEN}✓ PASSED{Colors.RESET}" if results['http'] == 0 else f"{Colors.RED}✗ FAILED{Colors.RESET}"
        print(f"HTTP Testy:                   {http_status}")
    
    print(f"\nCelkový čas vykonávania: {duration:.2f}s")
    
    # Celkový výsledok
    all_passed = all(r == 0 for r in results.values() if r is not None)
    
    if all_passed:
        print(f"\n{Colors.GREEN}{Colors.BOLD}{'='*70}{Colors.RESET}")
        print(f"{Colors.GREEN}{Colors.BOLD}{'VŠETKY TESTY PREŠLI!':^70}{Colors.RESET}")
        print(f"{Colors.GREEN}{Colors.BOLD}{'='*70}{Colors.RESET}\n")
        return 0
    else:
        print(f"\n{Colors.RED}{Colors.BOLD}{'='*70}{Colors.RESET}")
        print(f"{Colors.RED}{Colors.BOLD}{'NIEKTORÉ TESTY ZLYHALI':^70}{Colors.RESET}")
        print(f"{Colors.RED}{Colors.BOLD}{'='*70}{Colors.RESET}\n")
        return 1

if __name__ == '__main__':
    exit_code = main()
    sys.exit(exit_code)
