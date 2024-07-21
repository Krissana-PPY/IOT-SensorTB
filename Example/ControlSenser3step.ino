#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <math.h>

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

// MQTT topics
const char* reverse_topic = "reverse";
const char* forward_topic = "forward";
const char* setup1p_topic = "setup1p";
const char* setup2p_topic = "setup2p";
const char* setup3p_topic = "setup3p";

// Stepper motor settings
#define PUL_PIN 25  // Pulse pin
#define DIR_PIN 26  // Direction pin
#define ENA_PIN 27  // Enable pin

// Other settings
#define RXD2 16
#define TXD2 17
bool blinkState = false;
float rotation;
float facing_up;
String stringOne;
float distanceOfnumber;
int steps;
//float step_fraction = 0.0;

// Function declarations
void mpu_setup();
void laser_measure();
void laser_measure1();
void mpu_measure();
void reconnect_mqtt();
void callback(char* topic, byte* payload, unsigned int length);
void control_stepper_motor(float distance);
float parse_distance(const String &str);

void setup() {
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  stringOne = String("Sensor ");

  WiFi.softAP(ssid, password);
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);

  reconnect_mqtt();
  Serial2.write("O");

  mpu_setup();

  // Initialize stepper motor
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable the driver
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

  if (String(topic) == reverse_topic) {
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);
    client.publish("reset", payload);

  } else if (String(topic) == forward_topic) {
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);
    client.publish("next", payload);

  }  else if (String(topic) == setup1p_topic) {
    mpu_setup();
    Serial2.write("O");
    Serial2.write("D");
//    Serial.println(distanceOfnumber);
    digitalWrite(ENA_PIN, LOW);
    digitalWrite(DIR_PIN, HIGH); 
    delay(1000);
//    Serial.println(steps);
    stepMotor(steps);
    stepMotor(steps);
    delay(1000);
    laser_measure();
    laser_measure1();
    digitalWrite(DIR_PIN, LOW); 
    stepMotor(steps);
    stepMotor(steps);
    delay(1000);
//    step_fraction = 0.0;
  } else if (String(topic) == setup2p_topic) {
    mpu_setup();
    Serial2.write("O");
    Serial2.write("D");
    laser_measure();
    laser_measure1();
//    Serial.println(distanceOfnumber);
    control_stepper_motor(distanceOfnumber);
    digitalWrite(ENA_PIN, LOW);
    digitalWrite(DIR_PIN, HIGH); 
    delay(1000);
//    Serial.println(steps);
    stepMotor(steps);
    delay(1000);
    laser_measure();
    laser_measure1();
    delay(1000);
    digitalWrite(DIR_PIN, LOW); 
    stepMotor(steps);
    delay(1000);
//    step_fraction = 0.0;
  } else if (String(topic) == setup3p_topic) {
    mpu_setup();
    Serial2.write("O");
    Serial2.write("D");
    laser_measure();
    laser_measure1();
//    Serial.println(distanceOfnumber);
    control_stepper_motor(distanceOfnumber);
    digitalWrite(ENA_PIN, LOW);
    digitalWrite(DIR_PIN, HIGH); 
    delay(1000);
//    Serial.println(steps);
    stepMotor(steps);
    delay(1000);
    laser_measure();
    laser_measure1();
    delay(1000);
    stepMotor(steps);
    delay(1000);
    laser_measure();
    laser_measure1();
    delay(1000);
    digitalWrite(DIR_PIN, LOW); 
    stepMotor(steps);
    stepMotor(steps);
    delay(1000);
//    step_fraction = 0.0;
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
  distanceOfnumber = parse_distance(stringOne);
//  doc["angle_rad"] = atan(0.8 / distanceOfnumber);
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
  client.publish(mqtt_topic, payload);
  distanceOfnumber = parse_distance(stringOne); // Convert stringOne to float distance
//  doc["angle_rad"] = atan(0.8 / distanceOfnumber);
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
      client.subscribe(reverse_topic);
      client.subscribe(forward_topic);
      client.subscribe(setup1p_topic);
      client.subscribe(setup2p_topic);
      client.subscribe(setup3p_topic);
    } else {
      Serial.print("Failed to connect to MQTT. State: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void control_stepper_motor(float distanceOfnumber) {
  if (distanceOfnumber > 0) {
//    Serial.println(distanceOfnumber);
    float angle_rad = atan(80.0 / (distanceOfnumber * 100.0));
//    Serial.println(angle_rad);
    float angle_deg = angle_rad * 180 / M_PI;
//    Serial.println(angle_deg);
    angle_deg = floor(angle_deg * 10.0);
//    Serial.println(angle_deg); // Round to 1 decimal place
//    float total_steps = angle_deg / 0.1125 + step_fraction;
//    steps = (int)total_steps;
//    step_fraction = total_steps - steps;
    steps = round(angle_deg * 10) / 10.0;
//    Serial.println(steps);
  }
}

float parse_distance(const String &str) {
  int start = str.indexOf(":") + 1;
  int end = str.indexOf("m");
  String distanceStr = str.substring(start, end);
//  Serial.println(distanceStr);
  return distanceStr.toFloat();
}

void stepMotor(int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(500);
  }
}
