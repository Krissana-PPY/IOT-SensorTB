from flask import render_template, request, jsonify, send_from_directory
from config import *
from helpers import *
from mqtt_handlers import *
from socketio_handlers import *

def register_routes(app, mqtt):
    """
    Register all Flask routes for the application.
    """
    @app.route("/")
    def index():
        """Render the main HTML page."""
        return render_template("index.html")

    @app.route("/test_control_sensor.html")
    def test_control_sensor():
        """Render the test control sensor HTML page."""
        return render_template("test_control_sensor.html")

    @app.route('/send_command', methods=['POST'])
    def send_command():
        """Handle sending commands via MQTT."""
        data = request.get_json()
        topic = data.get('topic')
        if topic:
            try:
                mqtt.publish(topic, '')
                return jsonify({'status': 'success'}), 200
            except Exception as e:
                import logging
                logging.error(f"Error publishing topic {topic}: {e}")
                return jsonify({'status': 'error', 'message': 'Failed to publish topic'}), 500
        return jsonify({'status': 'error', 'message': 'Invalid topic'}), 400

    @app.route('/service-worker.js')
    def service_worker():
        """Serve the service worker file at the root."""
        return send_from_directory(app.static_folder, 'service-worker.js')
