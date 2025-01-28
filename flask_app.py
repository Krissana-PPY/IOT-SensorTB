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

# Global Variables to store sensor data and results
distance = []
angle_x = []
angle_y = []
each_pallet = []
distace_true = []
pallet_NO = 1
count_number_pallet = 1
count_number_ID = 1
check = True
MAX_RANGE = 15  # meters

# List of pallets identifiers
def generate_pallet_name(count_number_ID, count_number_pallet):
  name_ID = chr(64 + count_number_ID)
  return f"A{name_ID}{count_number_pallet:03}"

def create_app():
    # Create a Flask application
    app = Flask(__name__, template_folder='www/')
    app.config['SECRET_KEY'] = 'secret_key'

    # MySQL Configuration
    app.config['MYSQL_HOST'] = 'localhost'
    app.config['MYSQL_USER'] = 'root'
    app.config['MYSQL_PASSWORD'] = '1234'
    #app.config['MYSQL_PASSWORD'] = 'admintbl'
    app.config['MYSQL_DB'] = 'mydb'
    
    return app

app = create_app()
socketio = SocketIO(app)

# MQTT Configuration
app.config['MQTT_BROKER_URL'] = '192.168.50.94'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 300
app.config['MQTT_TLS_ENABLED'] = False
topic = "+"
mqtt = Mqtt(app, connect_async=True)

# Helper Functions to structure data for sending to the web interface
def send_pallet_to_web(np):
    return {"pallet_v": np} # All pallet

def send_point_to_web(dt):
    return {'point': dt} # EACH_pallet

def send_all_zero():
    return {'pallet_v': 0, 'point': 0} # All cls

def send_point_zero():
    return {'point': 0} # cls  EACH_pallet

def send_c_row(row):
    return {'c_row_v': row} # Now Row ID

def send_row(row):
    return {'row_v': row} # SS Row ID

def show_pallet(index1, index2):
    return generate_pallet_name(index1, index2) # Row ID

def reset_to_one(value):
  return 1 if value == 0 else value

def clear_data(input_str):
    # Extracts numeric values from the input string
    return re.findall(r'[-+]?\d*\.\d+|\d+', input_str)

def convert_angle(input_angle):
    return float(input_angle * (math.pi / 180))

# Collects and appends data from the input string to the respective lists
def collected_data(input_str):
    data = clear_data(input_str)
    angle_x.append(float(data[0]))
    angle_y.append(float(data[1]))
    distance.append(float(data[2]))

# Calculates the number of pallets based on the distance measure
def cal_pallet(distance_measure):
    pallet = math.floor((MAX_RANGE - distance_measure) / 1.215)
    print(pallet)
    each_pallet.append(pallet)
    return pallet

# Calculates the true distance using the distance and angle
def cal_distacetrue (distance_measure, angle_y):
    adj_distance = distance_measure * math.cos(convert_angle(angle_y))
    distace_true.append(adj_distance)

# Calculates the average true distance
def calculate_average_distance():
    if distace_true:
        return sum(distace_true) / len(distace_true)
    return 0

# Sums the number of pallets in the each_pallet list
def sum_pallet():
    return sum(math.floor(p) for p in each_pallet)

# MySQL Functions to interact with the database
# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

@contextmanager
def get_db_connection():
    try:
        connection = mysql.connector.connect(
            host=app.config['MYSQL_HOST'],
            user=app.config['MYSQL_USER'],
            password=app.config['MYSQL_PASSWORD'],
            database=app.config['MYSQL_DB']
        )
        yield connection
    except Error as err:
        logging.error(f"Database connection error: {err}")
        yield None
    finally:
        if 'connection' in locals() and connection and connection.is_connected():
            connection.close()

def execute_query(query, params=None, fetch=False):
    try:
        with get_db_connection() as connection:
            if connection:
                with connection.cursor() as cursor:
                    cursor.execute(query, params)
                    if fetch:
                        return cursor.fetchall()
                    connection.commit()
    except Error as err:
        logging.error(f"Database query error: {err}")
        return None
    
def send_EACH_PALLET(row_name, pallet_no, distance, angle_x, angle_y):
    date = datetime.now().strftime("%Y-%m-%d")
    query = """
        INSERT INTO ROW_EACH_PALLET 
        (ROW_ID, PALLET_NO, DISTANCE, ANGLE_X, ANGLE_Y, UPDATE_DATE) 
        VALUES (%s, %s, %s, %s, %s, %s)
    """
    execute_query(query, (row_name, pallet_no, distance, angle_x, angle_y, date))
    logging.info("Inserted ROW_EACH_PALLET data")

def send_DELETE(row_name):
    date = datetime.now().strftime("%Y-%m-%d")
    queries = [
        "DELETE FROM ROW_EACH_PALLET WHERE ROW_ID = %s AND UPDATE_DATE = %s",
        "DELETE FROM ROW_PALLET WHERE ROW_ID = %s AND UPDATE_DATE = %s",
        "DELETE FROM STOCK WHERE ROW_ID = %s AND UPDATE_DATE = %s"
    ]
    for query in queries:
        execute_query(query, (row_name, date))
    logging.info(f"Deleted data for ROW_ID: {row_name}")

def send_ROW_ID(row_name):
    date = datetime.now().strftime("%Y-%m-%d")
    queries = [
        "INSERT INTO STOCK (ROW_ID, UPDATE_DATE) VALUES (%s, %s)",
        "INSERT INTO ROW_PALLET (ROW_ID, UPDATE_DATE) VALUES (%s, %s)"
    ]
    for query in queries:
        execute_query(query, (row_name, date))
    logging.info(f"Inserted ROW_ID data: {row_name}")

def send_STOCK_UPDATE(row_name,pallet):
    query = """
        UPDATE STOCK 
        SET PALLET = %s 
        WHERE ROW_ID = %s AND UPDATE_DATE = %s
    """
    date = datetime.now().strftime("%Y-%m-%d")
    execute_query(query, (pallet, row_name, date))
    logging.info(f"Updated STOCK for ROW_ID: {row_name}")

def send_ROW_PALLET(row_name, pallet_no, each_pallet):
    date = datetime.now().strftime("%Y-%m-%d")
    if pallet_no > 1:
        query = """
            INSERT INTO ROW_PALLET 
            (ROW_ID, PALLET_NO, EACH_PALLET, UPDATE_DATE) 
            VALUES (%s, %s, %s, %s)
        """
        execute_query(query, (row_name, pallet_no, each_pallet, date))
    else:
        query = """
            UPDATE ROW_PALLET 
            SET PALLET_NO = %s, EACH_PALLET = %s 
            WHERE ROW_ID = %s AND UPDATE_DATE = %s
        """
        execute_query(query, (pallet_no, each_pallet, row_name, date))
    logging.info(f"Processed ROW_PALLET for ROW_ID: {row_name}")
     
# MQTT Handlers to process incoming messages
@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    print("connect with result code " + str(rc))
    client.subscribe(topic)

@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    global count_number_pallet, pallet_NO, count_number_ID, check
    msg = message.payload.decode()
    print(message.topic + " " + str(msg))
    socketio.emit('send_c_row', send_c_row(show_pallet(count_number_ID,count_number_pallet)), namespace='/')

    if message.topic == "measure":
        if check == True :
            send_ROW_ID(show_pallet(count_number_ID,count_number_pallet))
        check = False
        # Collect data from the message payload
        collected_data(msg)
        send_EACH_PALLET(show_pallet(count_number_ID,count_number_pallet), pallet_NO, distance[-1], angle_x[-1], angle_y[-1])
        
    elif message.topic == "finish":
         # Calculate true distances and average distance
        for i in range(len(distance)):
            cal_distacetrue(distance[i], angle_y[i])
        average_distance = calculate_average_distance()
        # Calculate number of pallets based on the average distance
        cal_pallet(average_distance)    
        for i in range(1, len(each_pallet) + 1):
            event_name = f'send_point_{i}'
            data = send_point_to_web(each_pallet[i - 1])  # ส่งข้อมูลของพาเลทที่เกี่ยวข้อง
            socketio.emit(event_name, data, namespace='/')
        
        # Store the last measurement in the database
        send_ROW_PALLET(show_pallet(count_number_ID,count_number_pallet), pallet_NO, each_pallet[-1])
        pallet_NO += 1 
        distance.clear()
        angle_x.clear()
        angle_y.clear()
        distace_true.clear()   

    elif message.topic == "NEXT_ROW":
        # Calculate total number of pallets and store in the database
        total_pallets = sum_pallet()
        print(f"Total pallets for {show_pallet(count_number_ID,count_number_pallet)}: {total_pallets}")
        send_STOCK_UPDATE(show_pallet(count_number_ID,count_number_pallet),total_pallets)
        send_pallet_to_web(total_pallets)
         # Clear the data lists and move to the next pallet
        distance.clear()
        angle_x.clear()
        angle_y.clear()
        each_pallet.clear()
        distace_true.clear()
        pallet_NO = 1
        count_number_pallet += 1 
        check = True
        socketio.emit('send_c_row', send_c_row(show_pallet(count_number_ID,count_number_pallet)), namespace='/')
        socketio.emit('send_row', send_row(show_pallet(count_number_ID,count_number_pallet-1)), namespace='/')
        socketio.emit('send_pallet', send_pallet_to_web(total_pallets), namespace='/')
        socketio.emit('set_point_zero', send_point_zero(), namespace='/')
    
    elif message.topic == "NEXT_ZONE":
        count_number_pallet = 1
        count_number_ID += 1
        socketio.emit('send_c_row', send_c_row(show_pallet(count_number_ID,count_number_pallet)), namespace='/')
        socketio.emit('set_point_zero', send_point_zero(), namespace='/')

    elif message.topic == "B":
        # Go back to the previous pallet and delete data from the database
        send_DELETE(show_pallet(count_number_ID,count_number_pallet))
        pallet_NO -= 1 #######################################################################################
        if count_number_pallet == 1 :
            count_number_ID = reset_to_one(count_number_ID - 1)
        count_number_pallet = reset_to_one(count_number_pallet - 1)
        print(show_pallet(count_number_ID,count_number_pallet))
        check = True
        distance.clear()
        angle_x.clear()
        angle_y.clear()
        each_pallet.clear()
        distace_true.clear()
        socketio.emit('send_c_row', send_c_row(show_pallet(count_number_ID,count_number_pallet)), namespace='/')
        socketio.emit('set_zero', send_all_zero(), namespace='/')

# SocketIO Handlers to communicate with the client
@socketio.on('connect')
def handle_connect():
    print('Client connected')
    socketio.emit('message', 'Connected to server')

@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')
    socketio.emit('message', 'state: disconnected')

@socketio.on('message')
def handle_message(message):
    print('Received message:', message)

# Route to render the main HTML page
@app.route("/")
def index():
    return render_template("index.html")

if __name__ == "__main__":
    socketio.run(app, host='0.0.0.0', port=5000, allow_unsafe_werkzeug=True)