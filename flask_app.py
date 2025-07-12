from flask import Flask, render_template, request, jsonify, send_from_directory
from flask_socketio import SocketIO, emit
from flask_mqtt import Mqtt
import math
import re
from datetime import datetime
import logging
import requests

# Global variables to store sensor data and results
distance = []  # List to store measured distances
distance_true = []  # List to store calculated straight-line distances
angle_x = []  # List to store angles along the X-axis
angle_y = []  # List to store angles along the Y-axis
sub_pallet = []  # List to store the number of pallets in each sub-row
sub_row = 1  # Current sub-row number in a pallet row
current_id = 0  # Identifier for the current pallet

last_row_id = None
last_max_range_id = None
last_max_range_value = None

url = ""

def CreateApp():
    """Create and configure the Flask application."""
    app = Flask(__name__, template_folder='www/')
    app.config['SECRET_KEY'] = 'secret_key'

    return app

app = CreateApp()
socketio = SocketIO(app)  # Initialize SocketIO for real-time communication

# MQTT Configuration
app.config['MQTT_BROKER_URL'] = '192.168.4.100'  # MQTT broker URL
app.config['MQTT_BROKER_PORT'] = 1883  # MQTT broker port
app.config['MQTT_USERNAME'] = ''  # MQTT username (if required)
app.config['MQTT_PASSWORD'] = ''  # MQTT password (if required)
app.config['MQTT_KEEPALIVE'] = 300  # Keep-alive interval for MQTT connection
app.config['MQTT_TLS_ENABLED'] = False  # Disable TLS for MQTT connection
topic = "+"  # MQTT topic to subscribe to
mqtt = Mqtt(app, connect_async=True)  # Initialize MQTT with asynchronous connection

# Helper functions to structure data for sending to the web interface
def FormatPalletData(np):  # Total number of pallets
    """Format pallet data for web interface."""
    return {"pallet_v": np}

def FormatPointData(dt):  # Result for each pallet
    """Format point data for web interface."""
    return {'point': dt}

def FormatAllZero():  # Clear total number of pallets
    """Format zero data for web interface."""
    return {'pallet_v': 0, 'point': 0}

def FormatPointZero():  # Clear data for each pallet
    """Format zero point data for web interface."""
    return {'point': 0}

def FormatCurrentRow(row):  # Name and ID of the pallet
    """Format current row data for web interface."""
    return {'c_row_v': row}

def FormatRow(row):  # Clear name and ID of the previous pallet
    """Format row data for web interface."""
    return {'row_v': row}

def ExtractNumericValues(input_str):
    """Extract numeric values from the input string."""
    return re.findall(r'[-+]?\d*\.\d+|\d+', input_str)

def CollectSensorData(input_str):
    """Collect and append data from the input string to the respective lists."""
    data = ExtractNumericValues(input_str)
    angle_x.append(float(data[0]))
    angle_y.append(float(data[1]))
    distance.append(float(data[2]))

def ConvertAngle(input_angle):
    """Convert angle from degrees to radians."""
    return float(input_angle * (math.pi / 180))

def CalDistacetrue(distance_measure, angle_y_val):
    """Calculate the true distance using the distance and angle."""
    adj_distance = distance_measure * math.cos(ConvertAngle(angle_y_val))
    distance_true.append(adj_distance)

def CalculateAverageDistance():
    """Calculate the average true distance."""
    if distance_true:
        # Check if there are values that differ from others by more than 0.6
        valid_distances = []
        for d in distance_true:
            # Check if the value d is close to at least one other value
            close_values = [other for other in distance_true if abs(d - other) <= 0.6 and d != other]
            if len(close_values) > 0:
                valid_distances.append(d)

        # If no values pass the condition, return -1
        if len(valid_distances) == 0:
            avg = -1
        else:
            # Calculate the average
            avg = sum(valid_distances) / len(valid_distances)

        return avg
    return 0

def CalResultPallet(average_distance):
    """Calculate the number of pallets based on the average distance."""
    max_range = (GetMaxRange(current_id) * 1.2) + 4.2
    pallet_calc = (max_range - average_distance) / 1.215
    if pallet_calc < 0.8:
        pallet = 1
    else:
        pallet = math.floor(pallet_calc)
    sub_pallet.append(pallet)
    return pallet

def HandleNextRowOrQR(row_id):
    """Handle logic when receiving NEXT_ROW or QR message."""
    pallets_total = SumPallet()
    print(f"Total pallets for {row_id} : {pallets_total}")
    socketio.emit('send_row', FormatRow(row_id), namespace='/')
    socketio.emit('send_pallet', FormatPalletData(pallets_total), namespace='/')
    socketio.emit('set_point_zero', FormatPointZero(), namespace='/')
    FormatPalletData(pallets_total)
    PostStock(row_id, pallets_total)
    ClearLists(sub_pallet)
    global sub_row, current_id
    sub_row = 1
    current_id += 1
    return GetFirstRowId(current_id)

def SumPallet():
    """Sum the number of pallets in the sub_pallet list."""
    return sum(math.floor(p) for p in sub_pallet if p >= 0)

def ClearLists(*lists):
    """Clear all lists passed as arguments."""
    for l in lists:
        l.clear()

def GetFirstRowId(current_id):
    """Get the first ROW_ID and NumberOfPages from the REST API."""
    api_url = url + f"Location/{current_id}"
    try:
        response = requests.get(api_url, verify=False)
        response.raise_for_status()
        data = response.json()

        row_id_val = data.get("rowId")
        number_of_pages = data.get("numberOfPages")
        return row_id_val, number_of_pages
    except Exception as e:
        logging.error(f"Error fetching ROW_ID and NumberOfPages from API: {e}")
        return None, None

def GetMaxRange(current_id):
    """Get the MAX_RANGE from the REST API, cache by id."""
    global last_max_range_id, last_max_range_value
    if last_max_range_id == current_id and last_max_range_value is not None:
        return last_max_range_value
    api_url = url + f"Location/{current_id}"
    try:
        response = requests.get(api_url, verify=False)
        response.raise_for_status()
        data = response.json()

        last_max_range_id = current_id
        last_max_range_value = data.get("area")
        return last_max_range_value
    except Exception as e:
        logging.error(f"Error fetching MAX_RANGE from API: {e}")
        return None

def PostEachPallet(row_id, sub_row_val, distance_val, angle_x_val, angle_y_val):
    """Send each pallet data as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url = url + "RowEachPallet"

    payload = {
        "rowId": row_id,
        "palletNo": sub_row_val,
        "distance": distance_val,
        "angleX": angle_x_val,
        "angleY": angle_y_val,
        "updateDate": current_date
    }
    try:
        response = requests.post(api_url, json=payload, verify=False)
        response.raise_for_status()
        print(response)
        logging.info(f"POST each_pallet for ROW_ID: {row_id}, SUB_ROW: {sub_row_val}.")
    except Exception as e:
        logging.error(f"Error POST each_pallet: {e}")

def PostRowPallet(row_id, sub_row_val, sub_pallet_val):
    """Send ROW_PALLET data as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url = url + "RowPallet"

    if sub_pallet_val < 0:
        sub_pallet_val = -1
    payload = {
        "rowId": row_id,
        "palletNo": sub_row_val,
        "eachPallet": sub_pallet_val,
        "updateDate": current_date
    }
    try:
        response = requests.post(api_url, json=payload, verify=False)
        response.raise_for_status()
        print(response)
        logging.info(f"POST ROW_PALLET for ROW_ID: {row_id}, SUB_ROW: {sub_row_val}.")
    except Exception as e:
        logging.error(f"Error POST ROW_PALLET: {e}")

def PostStock(row_id, pallets_total):
    """Send STOCK update as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url = url + "Stock"

    payload = {
        "rowId": row_id,
        "pallet": pallets_total,
        "updateDate": current_date
    }
    try:
        response = requests.post(api_url, json=payload, verify=False)
        response.raise_for_status()
        print(response)
        logging.info(f"POST STOCK update for ROW_ID: {row_id}.")
    except Exception as e:
        logging.error(f"Error POST STOCK update: {e}")

def DeleteRow(row_id):
    """Send delete row command as JSON via HTTP POST."""
    current_date = datetime.now().strftime("%Y-%m-%d")
    api_url =  url + "RowDelete"
    payload = {
        "type": "delete_row",
        "row_id": row_id,
        "update_date": current_date
    }
    try:
        response = requests.post(api_url, json=payload, verify=False)
        response.raise_for_status()
        logging.info(f"POST delete row for ROW_ID: {row_id}.")
    except Exception as e:
        logging.error(f"Error POST delete row: {e}")

# MQTT Handlers to process incoming messages
@mqtt.on_connect()
def HandleConnect(client, userdata, flags, rc):
    """Handle MQTT connection event."""
    print("connect with result code " + str(rc))
    client.subscribe(topic)

@mqtt.on_message()
def HandleMqttMessage(client, userdata, message):
    """Handle incoming MQTT messages."""
    global sub_row, current_id, last_row_id
    msg = message.payload.decode()
    print(message.topic + " " + str(msg))
    if current_id > 0:
        if last_row_id is None or current_id != getattr(HandleMqttMessage, 'last_id', None):
            row_id, page = GetFirstRowId(current_id)
            HandleMqttMessage.last_id = current_id
            last_row_id = row_id
        else:
            row_id = last_row_id

    if message.topic == "measure":
        CollectSensorData(msg)

    elif message.topic == "finish":

        for i in range(len(distance)):
            CalDistacetrue(distance[i], angle_y[i])
        average_distance = CalculateAverageDistance()
        PostEachPallet(row_id, sub_row, average_distance, 1, 1)

        if average_distance > 0:
            CalResultPallet(average_distance)
        else :
            sub_pallet.append(0)

        for i in range(1, len(sub_pallet) + 1):
            event_name = f'send_point_{i}'
            data = FormatPointData(sub_pallet[i - 1])
            socketio.emit(event_name, data, namespace='/')

        PostRowPallet(row_id, sub_row, sub_pallet[-1])
        ClearLists(distance, angle_x, angle_y, distance_true)
        sub_row += 1

    elif message.topic == "NoProducts":
        # Handle the case when there are no products
        print("No products detected")
        sub_pallet.append(0)
        PostRowPallet(row_id, sub_row, sub_pallet[-1])

        ClearLists(distance, angle_x, angle_y, distance_true)
        sub_row += 1

    elif message.topic == "F":
        row_id, page = GetFirstRowId(current_id)
        if row_id:
            socketio.emit('send_c_row', {'c_row_v': row_id})

    elif message.topic == "QR":
        if current_id == 0:
            current_id += 1
            print("START")
        elif current_id > 0:
            row_id, page = HandleNextRowOrQR(row_id)
            socketio.emit('send_c_row', {'c_row_v': row_id})

    elif message.topic == "B":
        ClearLists(distance, angle_x, angle_y, distance_true, sub_pallet)
        sub_row = 1
        current_id -= 1
        if current_id <= 1:
            current_id = 1
        row_id, page = GetFirstRowId(current_id)
        print(f"Going back to previous row: {row_id}")
        if row_id:
            socketio.emit('send_c_row', {'c_row_v': row_id})
        socketio.emit('set_zero', FormatAllZero(), namespace='/')

    elif message.topic == "T":
        row_id, page = GetFirstRowId(1)
        socketio.emit('send_c_row', {'c_row_v': row_id})
        print(f"Resetting to first row: {row_id}")

# SocketIO Handlers to communicate with the client
@socketio.on('connect')
def HandleConnect():
    """Handle client connection."""
    print('Client connected')
    socketio.emit('message', 'Connected to server')

@socketio.on('message')
def HandleMessage(message):
    """Handle incoming messages from the client."""
    print('Received message:', message)

# Routes to render HTML pages
@app.route("/")
def Index():
    """Render the main HTML page."""
    return render_template("index.html")

@app.route("/test_control_sensor.html")
def TestControlSensor():
    """Render the test control sensor HTML page."""
    return render_template("test_control_sensor.html")
@app.route('/index.html')
def index_html():
    return render_template("index.html")

@app.route('/service-worker.js')
def service_worker():
    return send_from_directory('static', 'service-worker.js')

@app.route('/send_command', methods=['POST'])
def SendCommand():
    """Handle sending commands via MQTT."""
    data = request.get_json()
    topic = data.get('topic')
    if topic:
        try:
            mqtt.publish(topic, '')
            return jsonify({'status': 'success'}), 200
        except Exception as e:
            logging.error(f"Error publishing topic {topic}: {e}")
            return jsonify({'status': 'error', 'message': 'Failed to publish topic'}), 500
    return jsonify({'status': 'error', 'message': 'Invalid topic'}), 400

if __name__ == "__main__":
    socketio.run(app, host='0.0.0.0', port=5000, allow_unsafe_werkzeug=True)