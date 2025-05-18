from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
from flask_mqtt import Mqtt
import math
import re
from datetime import datetime
import mysql.connector
from mysql.connector import Error
from contextlib import contextmanager
import logging

# Global variables to store sensor data and results
distance = []  # List to store measured distances
distace_true = []  # List to store calculated straight-line distances
angle_x = []  # List to store angles along the X-axis
angle_y = []  # List to store angles along the Y-axis
sub_pallet = []  # List to store the number of pallets in each sub-row
sub_row = 1  # Current sub-row number in a pallet row
id = 1  # Identifier for the current pallet
recheckpages = 0  # Counter for rechecking pages

def create_app():
    """Create and configure the Flask application."""
    app = Flask(__name__, template_folder='www/')
    app.config['SECRET_KEY'] = 'secret_key'
    
    return app

app = create_app()
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
def format_pallet_data(np):  # Total number of pallets
    """Format pallet data for web interface."""
    return {"pallet_v": np}

def format_point_data(dt):  # Result for each pallet
    """Format point data for web interface."""
    return {'point': dt}

def format_all_zero():  # Clear total number of pallets
    """Format zero data for web interface."""
    return {'pallet_v': 0, 'point': 0}

def format_point_zero():  # Clear data for each pallet
    """Format zero point data for web interface."""
    return {'point': 0}

def format_current_row(row):  # Name and ID of the pallet
    """Format current row data for web interface."""
    return {'c_row_v': row}

def format_row(row):  # Clear name and ID of the previous pallet
    """Format row data for web interface."""
    return {'row_v': row}

def extract_numeric_values(input_str):
    """Extract numeric values from the input string."""
    return re.findall(r'[-+]?\d*\.\d+|\d+', input_str)

def collect_sensor_data(input_str):
    """Collect and append data from the input string to the respective lists."""
    data = extract_numeric_values(input_str)
    angle_x.append(float(data[0]))
    angle_y.append(float(data[1]))
    distance.append(float(data[2]))

def convert_angle(input_angle):
    """Convert angle from degrees to radians."""
    return float(input_angle * (math.pi / 180))

def cal_distacetrue(distance_measure, angle_y):
    """Calculate the true distance using the distance and angle."""
    adj_distance = distance_measure * math.cos(convert_angle(angle_y))
    distace_true.append(adj_distance)

def calculate_average_distance():
    """Calculate the average true distance."""
    if distace_true:
        # Check if there are values that differ from others by more than 0.6
        valid_distances = []
        for d in distace_true:
            # Check if the value d is close to at least one other value
            close_values = [other for other in distace_true if abs(d - other) <= 0.6 and d != other]
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

def cal_result_pallet(average_distance):
    """Calculate the number of pallets based on the average distance."""
    MAX_RANGE = (get_MAX_RANGE(id) * 1.2) + 3
    pallet = math.floor((MAX_RANGE - average_distance) / 1.215)
    sub_pallet.append(pallet)
    return pallet

def sum_pallet():
    """Sum the number of pallets in the sub_pallet list."""
    return sum(math.floor(p) for p in sub_pallet if p >= 0)

def clear_lists(*lists):
    """Clear all lists passed as arguments."""
    for l in lists:
        l.clear()

# Global database connections
db_connections = []

def initialize_db_connections():
    """Initialize database connections at the start of the program."""
    global db_connections
    try:
        # Connection to the first database
        conn1 = mysql.connector.connect(
            host="localhost",
            user="root",
            password="1234",
            database="datasensor"
        )
        db_connections.append(conn1)
        print("Connected to localhost database (conn1).")

        # Connection to the second database
        conn2 = mysql.connector.connect(
            host="188.166.231.183",
            user="root",
            password="t295V37rQ%uS",
            database="datasensor1"
        )
        db_connections.append(conn2)
        print("Connected to server database (conn2).")

        logging.info("Database connections initialized successfully.")
    except Error as err:
        logging.error(f"Error initializing database connections: {err}")

def close_db_connections():
    """Close all database connections when the program exits."""
    global db_connections
    for conn in db_connections:
        if conn and conn.is_connected():
            conn.close()
    db_connections.clear()
    logging.info("Database connections closed.")

@contextmanager
def get_db_connections():
    """Provide the initialized database connections."""
    global db_connections
    yield db_connections

def ensure_db_connections():
    """Ensure all database connections are alive, reconnect if needed."""
    global db_connections
    for i, conn in enumerate(db_connections):
        try:
            if not conn.is_connected():
                conn.reconnect(attempts=3, delay=2)
        except Exception as e:
            logging.error(f"Error reconnecting to database {i}: {e}")

def get_first_row_id(id):
    """Get the first ROW_ID and NumberOfPages from the LOCATION table."""
    global db_connections
    ensure_db_connections()
    if db_connections and len(db_connections) > 0:
        try:
            cursor = db_connections[0].cursor()  # Use conn1
            cursor.execute("SELECT ROW_ID, NumberOfPages FROM LOCATION WHERE ID = %s", (id,))
            result = cursor.fetchone()
            cursor.close()
            return result if result else (None, None)
        except Error as e:
            logging.error(f"Error fetching ROW_ID and NumberOfPages: {e}")
            return None, None
    else:
        logging.error("Database connection (conn1) is not initialized.")
        return None, None

def get_MAX_RANGE(id):
    """Get the MAX_RANGE from the LOCATION table."""
    global db_connections
    ensure_db_connections()
    if db_connections and len(db_connections) > 0:
        try:
            cursor = db_connections[0].cursor()  # Use conn1
            cursor.execute("SELECT AREA FROM LOCATION WHERE ID = %s", (id,))
            MAX_RANGE = cursor.fetchone()
            cursor.close()
            return MAX_RANGE[0] if MAX_RANGE else None
        except Error as e:
            logging.error(f"Error fetching MAX_RANGE: {e}")
            return None
    else:
        logging.error("Database connection (conn1) is not initialized.")
        return None

def insert_stock_and_row_pallet(row_id):
    """Insert ROW_ID and current date into STOCK and ROW_PALLET tables in both databases."""
    global db_connections
    ensure_db_connections()
    current_date = datetime.now().strftime("%Y-%m-%d")
    queries = [
        (
            "INSERT INTO STOCK (ROW_ID, UPDATE_DATE) "
            "SELECT %s, %s FROM DUAL WHERE NOT EXISTS "
            "(SELECT 1 FROM STOCK WHERE ROW_ID = %s AND UPDATE_DATE = %s)",
            (row_id, current_date, row_id, current_date)
        ),
        (
            "INSERT INTO ROW_PALLET (ROW_ID, UPDATE_DATE) "
            "SELECT %s, %s FROM DUAL WHERE NOT EXISTS "
            "(SELECT 1 FROM ROW_PALLET WHERE ROW_ID = %s AND UPDATE_DATE = %s)",
            (row_id, current_date, row_id, current_date)
        )
    ]

    for connection in db_connections:
        try:
            cursor = connection.cursor()
            for query, params in queries:
                cursor.execute(query, params)
            connection.commit()
            cursor.close()
            logging.info(f"Inserted stock and row pallet for ROW_ID: {row_id} in one database.")
        except Error as e:
            logging.error(f"Error inserting stock and row pallet in one database: {e}")

def insernt_each_pallet(row_id, sub_row, distance, angle_x, angle_y):
    """Send data for each pallet to both databases."""
    global db_connections
    ensure_db_connections()
    current_date = datetime.now().strftime("%Y-%m-%d")
    query = (
        "INSERT INTO ROW_EACH_PALLET (ROW_ID, PALLET_NO, DISTANCE, ANGLE_X, ANGLE_Y, UPDATE_DATE) "
        "VALUES (%s, %s, %s, %s, %s, %s)"
    )
    params = (row_id, sub_row, distance, angle_x, angle_y, current_date)

    for connection in db_connections:
        try:
            cursor = connection.cursor()
            cursor.execute(query, params)
            connection.commit()
            cursor.close()
            logging.info(f"Inserted pallet data for ROW_ID: {row_id}, SUB_ROW: {sub_row} in one database.")
        except Error as e:
            logging.error(f"Error inserting pallet data in one database: {e}")

def insert_each_pallet_bulk(row_id, sub_row, distance_list, angle_x_list, angle_y_list):
    """Bulk insert data for each pallet to both databases."""
    global db_connections
    ensure_db_connections()
    current_date = datetime.now().strftime("%Y-%m-%d")
    values = [
        (row_id, sub_row, distance_list[i], angle_x_list[i], angle_y_list[i], current_date)
        for i in range(len(distance_list))
    ]
    query = (
        "INSERT INTO ROW_EACH_PALLET (ROW_ID, PALLET_NO, DISTANCE, ANGLE_X, ANGLE_Y, UPDATE_DATE) "
        "VALUES (%s, %s, %s, %s, %s, %s)"
    )
    for connection in db_connections:
        try:
            cursor = connection.cursor()
            cursor.executemany(query, values)
            connection.commit()
            cursor.close()
            logging.info(f"Bulk inserted pallet data for ROW_ID: {row_id}, SUB_ROW: {sub_row} in one database.")
        except Error as e:
            logging.error(f"Error bulk inserting pallet data in one database: {e}")

def insernt_ROW_PALLET(row_id, sub_row, sub_pallet):
    """Send data for each pallet to both databases."""
    global db_connections
    ensure_db_connections()
    current_date = datetime.now().strftime("%Y-%m-%d")
    for connection in db_connections:
        try:
            cursor = connection.cursor()
            if sub_pallet < 0:
                sub_pallet = -1
            if sub_row > 1:
                query = (
                    "INSERT INTO ROW_PALLET (ROW_ID, PALLET_NO, EACH_PALLET, UPDATE_DATE) "
                    "VALUES (%s, %s, %s, %s)"
                )
                params = (row_id, sub_row, sub_pallet, current_date)
            else:
                query = (
                    "UPDATE ROW_PALLET SET PALLET_NO = %s, EACH_PALLET = %s "
                    "WHERE ROW_ID = %s AND UPDATE_DATE = %s"
                )
                params = (sub_row, sub_pallet, row_id, current_date)
            cursor.execute(query, params)
            connection.commit()
            cursor.close()
            logging.info(f"Inserted/Updated ROW_PALLET for ROW_ID: {row_id}, SUB_ROW: {sub_row} in one database.")
        except Error as e:
            logging.error(f"Error inserting/updating ROW_PALLET in one database: {e}")

def update_STOCK_UPDATE(row_id, pallets_total):
    """Send data for pallet to database."""
    global db_connections
    ensure_db_connections()
    current_date = datetime.now().strftime("%Y-%m-%d")
    query = (
        "UPDATE STOCK SET PALLET = %s WHERE ROW_ID = %s AND UPDATE_DATE = %s"
    )
    params = (pallets_total, row_id, current_date)

    for connection in db_connections:
        try:
            cursor = connection.cursor()
            cursor.execute(query, params)
            connection.commit()
            cursor.close()
            logging.info(f"Updated STOCK for ROW_ID: {row_id} in one database.")
        except Error as e:
            logging.error(f"Error updating STOCK in one database: {e}")

def DELETE_ROW(row_id):
    """Delete data from the ROW PALLET table."""
    global db_connections
    ensure_db_connections()
    current_date = datetime.now().strftime("%Y-%m-%d")
    queries = [
        "DELETE FROM ROW_EACH_PALLET WHERE ROW_ID = %s AND UPDATE_DATE = %s",
        "DELETE FROM ROW_PALLET WHERE ROW_ID = %s AND UPDATE_DATE = %s",
        "DELETE FROM STOCK WHERE ROW_ID = %s AND UPDATE_DATE = %s"
    ]

    for connection in db_connections:
        try:
            cursor = connection.cursor()
            for query in queries:
                cursor.execute(query, (row_id, current_date))
            connection.commit()
            cursor.close()
            logging.info(f"Deleted data for ROW_ID: {row_id} in one database.")
        except Error as e:
            logging.error(f"Error deleting data for ROW_ID: {row_id} in one database: {e}")

# MQTT Handlers to process incoming messages
@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    """Handle MQTT connection event."""
    print("connect with result code " + str(rc))
    client.subscribe(topic)
    

@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    """Handle incoming MQTT messages."""
    global sub_row, id, recheckpages
    msg = message.payload.decode()
    print(message.topic + " " + str(msg))
    row_id, page = get_first_row_id(id)
    if row_id:
        socketio.emit('send_c_row', {'c_row_v': row_id})

    if message.topic == "measure":
        collect_sensor_data(msg)

    elif message.topic == "finish":
        for i in range(len(distance)):
            cal_distacetrue(distance[i], angle_y[i])
        average_distance = calculate_average_distance()
        if average_distance > 0:
            cal_result_pallet(average_distance)
            for i in range(1, len(sub_pallet) + 1):
                event_name = f'send_point_{i}'
                data = format_point_data(sub_pallet[i - 1])  # Send data for the relevant pallet
                socketio.emit(event_name, data, namespace='/')
        else :
            sub_pallet.append(0)
            for i in range(1, len(sub_pallet) + 1):
                event_name = f'send_point_{i}'
                data = format_point_data(-1)  # Send data for the relevant pallet
                socketio.emit(event_name, data, namespace='/')
        if distance:  # เพิ่มเช็คกรณีไม่มีข้อมูล
            insert_each_pallet_bulk(row_id, sub_row, distance, angle_x, angle_y)
        # Store the last measurement in the database
        insernt_ROW_PALLET(row_id, sub_row, sub_pallet[-1])
        clear_lists(distance, angle_x, angle_y, distace_true)
        sub_row += 1

    elif message.topic == "NoProducts":
        # Handle the case when there are no products
        print("No products detected")
        sub_pallet.append(0)
        insernt_ROW_PALLET(row_id, sub_row, sub_pallet[-1])  

        clear_lists(distance, angle_x, angle_y, distace_true)
        sub_row += 1

    elif message.topic == "F":
        row_id, page = get_first_row_id(id)
        if row_id:
            socketio.emit('send_c_row', {'c_row_v': row_id})
            insert_stock_and_row_pallet(row_id)

        recheckpages += 1
        if recheckpages > page:
            recheckpages = 1
            mqtt.publish("NEXT_ROW", "")
        
    elif message.topic == "NEXT_ROW":
        # Calculate total number of pallets and store in the database
        pallets_total = sum_pallet()
        print(f"Total pallets for {row_id} : {pallets_total}")
        socketio.emit('send_c_row', format_current_row(row_id), namespace='/')
        socketio.emit('send_row', format_row(row_id), namespace='/')
        socketio.emit('send_pallet', format_pallet_data(pallets_total), namespace='/')
        socketio.emit('set_point_zero', format_point_zero(), namespace='/')
        format_pallet_data(pallets_total)
        update_STOCK_UPDATE(row_id, pallets_total)
        clear_lists(distance, angle_x, angle_y, distace_true, sub_pallet)
        sub_row = 1
        id += 1
        row_id, page = get_first_row_id(id)
        insert_stock_and_row_pallet(row_id)
        
    #elif message.topic == "B":
    #    clear_lists(distance, angle_x, angle_y, distace_true, sub_pallet)
    #    sub_row = 1
    #    if sub_row <= 1:
    #        sub_row = 1
    #    id -= 1
    #    DELETE_ROW(row_id)
    #    if id <= 1:
    #        id = 1
    #    row_id, page = get_first_row_id(id)
    #    socketio.emit('send_c_row', format_current_row(row_id), namespace='/')
    #    socketio.emit('set_zero', format_all_zero(), namespace='/')
    
    elif message.topic == "START":
        print("Process started")
        mqtt.publish("F", "forward")

# SocketIO Handlers to communicate with the client
@socketio.on('connect')
def handle_connect():
    """Handle client connection."""
    print('Client connected')
    socketio.emit('message', 'Connected to server')

@socketio.on('message')
def handle_message(message):
    """Handle incoming messages from the client."""
    print('Received message:', message)

# Routes to render HTML pages
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
            logging.error(f"Error publishing topic {topic}: {e}")
            return jsonify({'status': 'error', 'message': 'Failed to publish topic'}), 500
    return jsonify({'status': 'error', 'message': 'Invalid topic'}), 400

if __name__ == "__main__":
    try:
        initialize_db_connections()  # Initialize database connections at startup
        print("All systems are ready. Application is now running.")
        socketio.run(app, host='0.0.0.0', port=5000, allow_unsafe_werkzeug=True)
    finally:
        close_db_connections()  # Ensure connections are closed on exit