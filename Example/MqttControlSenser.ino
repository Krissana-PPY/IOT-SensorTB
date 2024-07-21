#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MPU6050 settings
MPU6050 mpu;
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];
Quaternion q;
VectorInt16 aa, aaReal, aaWorld;
VectorFloat gravity;
float euler[3];
float ypr[3];
volatile bool mpuInterrupt = false;

// WiFi and MQTT settings
const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";
const char* mqtt_broker = "192.168.4.2";
const char* mqtt_client_id = "esp32-sensor";
const char* mqtt_topic = "measure";
WiFiClient espClient;
PubSubClient client(espClient);

// Button pins (redefined as MQTT topics)
const char* distance_topic = "distance";
const char* reverse_topic = "reverse";
const char* forward_topic = "forward";
const char* setmpu_topic = "setmpu";


// Other settings
#define RXD2 16
#define TXD2 17
bool blinkState = false;
float rotation;
float facing_up;
String stringOne;

// Function declarations
void mpu_setup();
void dp_oled(String msg);
void laser_measure();
void laser_measure1();
void mpu_measure();
void reconnect_mqtt();
void callback(char* topic, byte* payload, unsigned int length);

void setup() {
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  stringOne = String("Sensor ");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  WiFi.softAP(ssid, password);
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);

  reconnect_mqtt();
  Serial2.write("O");

  mpu_setup();
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == distance_topic) {
    laser_measure();
    laser_measure1();
  } else if (String(topic) == reverse_topic) {
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);

    if (client.publish("reset", payload)) {
      dp_oled("reverse");
    }
  } else if (String(topic) == forward_topic) {
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);

    if (client.publish("next", payload)) {
      dp_oled("forward");
    } else {
      dp_oled("fail");
    }
  } else if (String(topic) == setmpu_topic) {
    dp_oled("setup mpu");
    mpu_setup();
    Serial2.write("D");
  }
}

void mpu_setup() {
  Wire.begin();
  Wire.setClock(400000);
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788);

  if (devStatus == 0) {
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    mpu.setDMPEnabled(true);
    mpuIntStatus = mpu.getIntStatus();
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
}

void dp_oled(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println(msg);
  display.display();
}

void laser_measure() {
  mpu_measure();
  Serial2.write("D");
  delay(500);

  while (Serial2.available()) {
    char data = Serial2.read();
    stringOne += data;
  }
  Serial.print(stringOne);
  StaticJsonDocument<200> doc;
  doc["rotation"] = rotation;
  doc["facing_up"] = facing_up;
  doc["distance"] = stringOne;
  char payload[200];
  serializeJson(doc, payload);
  stringOne = "";
}

void laser_measure1() {
  mpu_measure();
  Serial2.write("D");
  delay(500);

  while (Serial2.available()) {
    char data = Serial2.read();
    stringOne += data;
  }
  Serial.print(stringOne);
  StaticJsonDocument<200> doc;
  doc["rotation"] = rotation;
  doc["facing_up"] = facing_up;
  doc["distance"] = stringOne;
  char payload[200];
  serializeJson(doc, payload);
  if (client.publish(mqtt_topic, payload)) {
    dp_oled(stringOne);
  }
  stringOne = "";
}

void mpu_measure() {
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    rotation = ypr[0] * 180 / M_PI;
    facing_up = ypr[2] * 180 / M_PI;
    delay(500);
  }
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Connected to MQTT");
      dp_oled("mqtt connect");
      client.subscribe(distance_topic);
      client.subscribe(reverse_topic);
      client.subscribe(forward_topic);
      client.subscribe(setmpu_topic);
    } else {
      Serial.print("Failed to connect to MQTT. State: ");
      Serial.println(client.state());
      dp_oled("failed to connect mqtt");
      delay(2000);
    }
  }
}