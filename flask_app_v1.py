from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
from flask_mqtt import Mqtt
import math
import re
from datetime import datetime
import mysql.connector

# Global Variables to store sensor data and results
distance = []
angle_x = []
angle_y = []
each_pallate = []
distace_true = []
pallate_NO = 1
count = 0
MAX_RANGE = 15  # meters

# List of pallates identifiers
pallates = [f"AA{i:03}" for i in range(1, 33)]

def create_app():
    # Create a Flask application
    app = Flask(__name__, template_folder='www/')
    app.config['SECRET_KEY'] = 'secret_key'

    # MySQL Configuration
    app.config['MYSQL_HOST'] = 'localhost'
    app.config['MYSQL_USER'] = 'root'
    app.config['MYSQL_PASSWORD'] = '1234'
    app.config['MYSQL_DB'] = 'mydb'
    
    return app

app = create_app()
socketio = SocketIO(app)

# MQTT Configuration
app.config['MQTT_BROKER_URL'] = '192.168.4.2'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 300
app.config['MQTT_TLS_ENABLED'] = False
topic = "+"
mqtt = Mqtt(app, connect_async=True)

# Helper Functions to structure data for sending to the web interface
def send_pallate_to_web(np):
    return {"pallate_v": np} # All pallate

def send_point_to_web(dt):
    return {'point': dt} # EACH_pallate

def send_all_zero():
    return {'pallate_v': 0, 'point': 0} # All cls

def send_point_zero():
    return {'point': 0} # cls  EACH_PALLATE

def send_c_row(row):
    return {'c_row_v': row} # Now Row ID

def send_row(row):
    return {'row_v': row} # SS Row ID

def show_pallate(index):
    return pallates[index] # Row ID

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

# Calculates the number of pallates based on the distance measure
def cal_pallate(distance_measure):
    pallate = math.floor((MAX_RANGE - distance_measure) / 1.215)
    print(pallate)
    each_pallate.append(pallate)
    return pallate

# Calculates the true distance using the distance and angle
def cal_distacetrue (distance_measure, angle_y):
    adj_distance = distance_measure * math.cos(convert_angle(angle_y))
    distace_true.append(adj_distance)

# Calculates the average true distance
def calculate_average_distance():
    if distace_true:
        return sum(distace_true) / len(distace_true)
    return 0

# Sums the number of pallates in the each_pallate list
def sum_pallate():
    return sum(math.floor(p) for p in each_pallate)

# MySQL Functions to interact with the database
def get_db_connection():
    try:
        connection = mysql.connector.connect(
            host=app.config['MYSQL_HOST'],
            user=app.config['MYSQL_USER'],
            password=app.config['MYSQL_PASSWORD'],
            database=app.config['MYSQL_DB']
        )
        return connection
    except mysql.connector.Error as err:
        print(f"Error: {err}")
        return None

def send_EACH_PALLATE(row_name, pallate_no, distance, angle_x, angle_y):
    try:
        connection = get_db_connection()
        if connection and connection.is_connected():
            current_time = datetime.now()
            date = current_time.strftime("%Y-%m-%d")
            cur = connection.cursor()
            cur.execute("""
                INSERT INTO ROW_EACH_PALLATE (ROW_ID, PALLATE_NO, DISTANCE, ANGLE_X, ANGLE_Y, UPDATE_DATE) 
                VALUES (%s, %s, %s, %s, %s, %s)
                """, (row_name, pallate_no, distance, angle_x, angle_y, date))
            connection.commit()
            print("Insert data")
            cur.close()
    except mysql.connector.Error as err:
        print(f"Database error: {err}")
        return "error"
    except Exception as e:
        print(f"Error: {e}")
        return "error"
    finally:
        if connection and connection.is_connected():
            connection.close()

def send_DELETE(row_name):
    try:
        connection = get_db_connection()
        if connection and connection.is_connected():
            cur = connection.cursor()
            current_time = datetime.now()
            date = current_time.strftime("%Y-%m-%d")

            # Check existence before delete
            cur.execute("DELETE FROM ROW_EACH_PALLATE WHERE ROW_ID = %s AND UPDATE_DATE = %s", (row_name, date))
            cur.execute("DELETE FROM ROW_PALLATE WHERE ROW_ID = %s AND UPDATE_DATE = %s", (row_name, date))
            cur.execute("DELETE FROM STOCK WHERE ROW_ID = %s AND UPDATE_DATE = %s", (row_name, date))
            connection.commit()

            cur.close()
    except mysql.connector.Error as err:
        print(f"Database error: {err}")
        return "error"
    except Exception as e:
        print(f"Error: {e}")
        return "error"
    finally:
        if connection and connection.is_connected():
            connection.close()

def send_STOCK(row_name, pallate):
    try:
        connection = get_db_connection()
        if connection and connection.is_connected():
            current_time = datetime.now()
            date = current_time.strftime("%Y-%m-%d") 
            cur = connection.cursor()
            cur.execute("""
                INSERT INTO STOCK (ROW_ID, PALLATE, UPDATE_DATE) 
                VALUES (%s, %s, %s)
                """, (row_name, pallate, date))
            connection.commit()
            print("Insert data")
            cur.close()
    except mysql.connector.Error as err:
        print(f"Database error: {err}")
        return "error"
    except Exception as e:
        print(f"Error: {e}")
        return "error"
    finally:
        if connection and connection.is_connected():
            connection.close()

def send_ROW_PALLATE(row_name, pallate_no, each_pallate):
    try:
        connection = get_db_connection()
        if connection and connection.is_connected():
            current_time = datetime.now()
            date = current_time.strftime("%Y-%m-%d")             
            cur = connection.cursor()
            cur.execute("""
                INSERT INTO ROW_PALLATE (ROW_ID, PALLATE_NO, EACH_PALLATE, UPDATE_DATE ) 
                VALUES (%s, %s, %s, %s)
                """, (row_name, pallate_no, each_pallate, date))
            connection.commit()
            print("Insert data")
            cur.close()
    except mysql.connector.Error as err:
        print(f"Database error: {err}")
        return "error"
    except Exception as e:
        print(f"Error: {e}")
        return "error"
    finally:
        if connection and connection.is_connected():
            connection.close()
     
# MQTT Handlers to process incoming messages
@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    print("connect with result code " + str(rc))
    client.subscribe(topic)

@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    global count, pallate_NO
    msg = message.payload.decode()
    print(message.topic + " " + str(msg))
    socketio.emit('send_c_row', send_c_row(show_pallate(count)), namespace='/')

    if message.topic == "measure":
        # Collect data from the message payload
        collected_data(msg)
        send_EACH_PALLATE(show_pallate(count), pallate_NO, distance[-1], angle_x[-1], angle_y[-1])
        
    elif message.topic == "finish":
         # Calculate true distances and average distance
        for i in range(len(distance)):
            cal_distacetrue(distance[i], angle_y[i])
        average_distance = calculate_average_distance()
        # Calculate number of pallates based on the average distance
        cal_pallate(average_distance)    
        send_point_to_web(each_pallate)
        print("each_pallate = ", each_pallate)
        point_events = {
            1: 'send_point_1',
            2: 'send_point_2',
            3: 'send_point_3',
            4: 'send_point_4',
            5: 'send_point_5'
        }

        if len(each_pallate) in point_events:
            socketio.emit(point_events[len(each_pallate)], send_point_to_web(each_pallate[-1]), namespace='/')
        
        # Store the last measurement in the database
        send_ROW_PALLATE(show_pallate(count), pallate_NO, each_pallate[-1])
        pallate_NO += 1    

    elif message.topic == "next":
        # Calculate total number of pallates and store in the database
        total_pallates = sum_pallate()
        print(f"Total pallates for {show_pallate(count)}: {total_pallates}")
        send_STOCK(show_pallate(count), total_pallates)
        send_pallate_to_web(total_pallates)
         # Clear the data lists and move to the next pallate
        distance.clear()
        angle_x.clear()
        angle_y.clear()
        each_pallate.clear()
        distace_true.clear()
        pallate_NO = 1
        count += 1 
        socketio.emit('send_c_row', send_c_row(show_pallate(count)), namespace='/')
        socketio.emit('send_row', send_row(show_pallate(count-1)), namespace='/')
        socketio.emit('send_pallate', send_pallate_to_web(total_pallates), namespace='/')
        socketio.emit('set_point_zero', send_point_zero(), namespace='/')

    elif message.topic == "reset":
        # Go back to the previous pallate and delete data from the database
        pallate_NO -= 1
        if pallate_NO == 0:
            pallate_NO = 1
        send_DELETE(show_pallate(count))
        count -= 1
        send_DELETE(show_pallate(count))
        print(show_pallate(count))
        distance.clear()
        angle_x.clear()
        angle_y.clear()
        each_pallate.clear()
        distace_true.clear()
        socketio.emit('send_c_row', send_c_row(show_pallate(count)), namespace='/')
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
    socketio.run(app, host='0.0.0.0', port=5000)