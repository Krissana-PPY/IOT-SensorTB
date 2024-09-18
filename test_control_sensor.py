from flask import Flask, render_template, request, jsonify
from flask_mqtt import Mqtt
import json

app = Flask(__name__)

# MQTT Configuration
app.config['MQTT_BROKER_URL'] = '192.168.4.2'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''  # Set username if your broker requires authentication
app.config['MQTT_PASSWORD'] = ''  # Set password if your broker requires authentication
app.config['MQTT_KEEPALIVE'] = 300  # Set KeepAlive time in seconds
app.config['MQTT_TLS_ENABLED'] = False  # Set to True if your broker supports TLS

mqtt = Mqtt(app, connect_async=True)

@app.route('/')
def index():
    return render_template('test_control_sensor.html')

@app.route('/send_command', methods=['POST'])
def send_command():
    data = request.json
    command = data.get('command')
    
    topic_map = {
        '2F': '2F',
        '3F': '3F',
        'UDF': 'UDF',
        'left': 'NEXT_ROW',
        'right': 'Revert',
        'up': 'NEXT_ZONE'
    }
    
    if command in topic_map:
        topic = topic_map[command]
        mqtt.publish(topic, 'triggered')
        return jsonify({"status": "success", "message": f"Sent {topic} command"}), 200
    else:
        return jsonify({"status": "error", "message": "Invalid command"}), 400

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)