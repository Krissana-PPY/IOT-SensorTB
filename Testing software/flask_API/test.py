import paho.mqtt.client as mqtt

# MQTT broker settings
broker = "192.168.4.100"
port = 1883

# Define the MQTT client
client = mqtt.Client("test_client")

# Connect to the MQTT broker
client.connect(broker, port)

# Function to publish a message to a given topic
def publish_message(topic, message):
    client.publish(topic, message)
    print(f"Published '{message}' to topic '{topic}'")

# Input command from user
while True:
    command = input("Enter command : ").strip().lower()

    # MQTT topics mapping
    topics = {
        "t": "test",
        "r": "reverse",
        "f": "forward",
        "ff": "2F",
        "fff": "3F",
        "ud": "UDF"

    }

    # Check if the command is exit
    if command == "exit":
        break

    # Check if the command is valid and publish
    if command in topics:
        topic = topics[command]
        publish_message(topic, command)
    else:
        print(f"Invalid command: {command}")

# Disconnect from the broker
client.disconnect()
