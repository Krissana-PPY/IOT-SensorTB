from flask import Flask
from flask_socketio import SocketIO
from flask_mqtt import Mqtt

def CreateApp():
    app = Flask(__name__, template_folder='www/')
    app.config['SECRET_KEY'] = 'secret_key'
    return app

app = CreateApp()
socketio = SocketIO(app)
app.config['MQTT_BROKER_URL'] = '192.168.4.100'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 300
app.config['MQTT_TLS_ENABLED'] = False
mqtt = Mqtt(app, connect_async=True)

from routes import register_routes
from mqtt_handlers import register_mqtt_handlers
from socketio_handlers import register_socketio_handlers

register_routes(app, mqtt)
register_mqtt_handlers(mqtt, socketio)
register_socketio_handlers(socketio)

if __name__ == "__main__":
    socketio.run(app, host='0.0.0.0', port=5000, allow_unsafe_werkzeug=True)