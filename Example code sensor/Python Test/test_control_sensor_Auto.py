from flask import Flask, render_template, request, jsonify
from flask_mqtt import Mqtt
import json
import time
import threading

app = Flask(__name__)

# MQTT Configuration
app.config['MQTT_BROKER_URL'] = '192.168.4.100'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''  # Set username if your broker requires authentication
app.config['MQTT_PASSWORD'] = ''  # Set password if your broker requires authentication
app.config['MQTT_KEEPALIVE'] = 300  # Set KeepAlive time in seconds
app.config['MQTT_TLS_ENABLED'] = False  # Set to True if your broker supports TLS

mqtt = Mqtt(app, connect_async=True)

@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print(f"Failed to connect, return code {rc}")

@mqtt.on_disconnect()
def handle_disconnect(client, userdata, rc):
    print("Disconnected from MQTT Broker")

@mqtt.on_log()
def handle_logging(client, userdata, level, buf):
    print(f"Log: {buf}")

@app.route('/')
def index():
    return render_template('test_control_sensor.html')

def publish_repeatedly(topic, rounds, interval):
    for _ in range(rounds):
        mqtt.publish(topic, 'triggered')
        time.sleep(interval)

@app.route('/send_command', methods=['POST'])
def send_command():
    data = request.json
    command = data.get('command')
    
    topic_map = {
        '2F': '2F',
        '3F': '3F',
        'UDF': 'UDF',
        'left': 'NEXT_ROW',
        'right': 'B',
        'up': 'NEXT_ZONE'
    }
    
    if command in topic_map:
        topic = topic_map[command]
        thread = threading.Thread(target=publish_repeatedly, args=(topic, 500, 30))
        thread.start()
        return jsonify({"status": "success", "message": f"Started sending {topic} command every 30 seconds for 500 rounds"}), 200
    else:
        return jsonify({"status": "error", "message": "Invalid command"}), 400

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5001)