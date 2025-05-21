from config import *
from helpers import *
from flask_socketio import SocketIO

def register_mqtt_handlers(mqtt, socketio: SocketIO):
    """
    Register all MQTT event handlers.
    """
    @mqtt.on_connect()
    def handle_connect(client, userdata, flags, rc):
        print("connect with result code " + str(rc))
        client.subscribe("+")

    @mqtt.on_message()
    def handle_mqtt_message(client, userdata, message):
        global sub_row, current_id, recheck_pages, last_row_id
        msg = message.payload.decode()
        print(message.topic + " " + str(msg))
        if last_row_id is None or current_id != getattr(handle_mqtt_message, 'last_id', None):
            row_id, page = GetFirstRowId(current_id)
            handle_mqtt_message.last_id = current_id
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
            else:
                sub_pallet.append(0)
            for i in range(1, len(sub_pallet) + 1):
                event_name = f'send_point_{i}'
                data = FormatPointData(sub_pallet[i - 1])
                socketio.emit(event_name, data, namespace='/')
            PostRowPallet(row_id, sub_row, sub_pallet[-1])
            ClearLists(distance, angle_x, angle_y, distance_true)
            sub_row += 1
        elif message.topic == "NoProducts":
            print("No products detected")
            sub_pallet.append(0)
            PostRowPallet(row_id, sub_row, sub_pallet[-1])
            ClearLists(distance, angle_x, angle_y, distance_true)
            sub_row += 1
        elif message.topic == "F":
            row_id, page = GetFirstRowId(current_id)
            if row_id:
                socketio.emit('send_c_row', {'c_row_v': row_id})
            recheck_pages += 1
            if recheck_pages > page:
                recheck_pages = 1
                mqtt.publish("NEXT_ROW", "")
        elif message.topic == "NEXT_ROW":
            pallets_total = SumPallet()
            print(f"Total pallets for {row_id} : {pallets_total}")
            socketio.emit('send_c_row', FormatCurrentRow(row_id), namespace='/')
            socketio.emit('send_row', FormatRow(row_id), namespace='/')
            socketio.emit('send_pallet', FormatPalletData(pallets_total), namespace='/')
            socketio.emit('set_point_zero', FormatPointZero(), namespace='/')
            FormatPalletData(pallets_total)
            PostStock(row_id, pallets_total)
            ClearLists(distance, angle_x, angle_y, distance_true, sub_pallet)
            sub_row = 1
            current_id += 1
            row_id, page = GetFirstRowId(current_id)
        elif message.topic == "B":
            ClearLists(distance, angle_x, angle_y, distance_true, sub_pallet)
            sub_row = 1
            current_id -= 1
            if current_id <= 1:
                current_id = 1
            row_id, page = GetFirstRowId(current_id)
            if row_id:
                socketio.emit('send_c_row', {'c_row_v': row_id})
            socketio.emit('set_zero', FormatAllZero(), namespace='/')
        elif message.topic == "START":
            print("Process started")
            mqtt.publish("F", "forward")
