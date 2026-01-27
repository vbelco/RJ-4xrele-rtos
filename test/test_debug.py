import paho.mqtt.client as mqtt
import ssl
import json
import time

broker = "dev4.ccsipro.sk"
port = 8883
topic_send = "system/vbelco/eth-liligo1"
topic_receive = "res/system/vbelco/eth-liligo1"

response = None

def on_message(client, userdata, msg):
    global response
    response = msg.payload.decode()
    print(f"Odpoveď: {response}")

client = mqtt.Client()
client.username_pw_set("prototip", "Rff5Gggs899")
client.tls_set(ca_certs="broker.crt", tls_version=ssl.PROTOCOL_TLSv1_2)
client.on_message = on_message
client.connect(broker, port, 60)
client.subscribe(topic_receive)
client.loop_start()

# Test 1: Zapnúť GATE1 na 3s
print("\n=== Test 1: Zapnutie GATE1 na 3000ms ===")
response = None
client.publish(topic_send, json.dumps({"a": "gate_ms", "g": "GATE1", "d": 3000}))
time.sleep(0.5)

# Skontroluj STATUS hneď po zapnutí
print("\n=== STATUS ihneď po zapnutí ===")
response = None
client.publish(topic_send, json.dumps({"a": "status"}))
time.sleep(0.5)
if response:
    status = json.loads(response)
    print(f"current_millis: {status.get('current_millis')}")
    print(f"gate_off_time: {status.get('gate_off_time')}")
    print(f"gate_status: {status.get('gate_status')}")

# Čakaj 3.5s
print("\n=== Čakám 3.5s ===")
time.sleep(3.5)

# Skontroluj STATUS po 3.5s
print("\n=== STATUS po 3.5s (má byť vypnuté) ===")
response = None
client.publish(topic_send, json.dumps({"a": "status"}))
time.sleep(0.5)
if response:
    status = json.loads(response)
    print(f"current_millis: {status.get('current_millis')}")
    print(f"gate_off_time: {status.get('gate_off_time')}")
    print(f"gate_status: {status.get('gate_status')}")

client.loop_stop()
client.disconnect()
