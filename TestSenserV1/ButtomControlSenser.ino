#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

#define OUTPUT_READABLE_YAWPITCHROLL
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define RXD2 (16)  // กำหนดขา RX ของ Serial1 เป็นขา 16
#define TXD2 (17)  // กำหนดขา TX ของ Serial1 เป็นขา 17

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

using namespace std;

//setup pin for button
int8_t measure_pin = 26;
int8_t reset_pin = 33;
int8_t next_pin = 27;
int8_t setmpu_pin = 14;

MPU6050 mpu;

bool blinkState = false;

//define variable for collect mpu data
float rotation;
float facing_up;
char sensor;
String stringOne;

bool dmpReady = false;   // set true if DMP init was successful
uint8_t mpuIntStatus;    // holds actual interrupt status byte from MPU
uint8_t devStatus;       // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;     // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;      // count of all bytes currently in FIFO
uint8_t fifoBuffer[64];  // FIFO storage buffer

Quaternion q;         // [w, x, y, z]         quaternion container
VectorInt16 aa;       // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;   // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;  // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;  // [x, y, z]            gravity vector
float euler[3];       // [psi, theta, phi]    Euler angle container
float ypr[3];         // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

uint8_t teapotPacket[14] = { '$', 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x00, '\r', '\n' };

volatile bool mpuInterrupt = false;  // indicates whether MPU interrupt pin has gone high



// WiFi settings
const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";

// MQTT settings
const char* mqtt_broker = "192.168.4.2";
const char* mqtt_client_id = "esp32-sensor";
const char* mqtt_topic = "measure";

// Create WiFi and MQTT client instances
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  //begin serial port 1 buad 115200 and serial port 2 
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  delay(100);
  Serial.flush();

  stringOne = String("Sensor ");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    for (;;)
      ;
  }

  pinMode(measure_pin, INPUT);
  pinMode(reset_pin, INPUT);
  pinMode(next_pin, INPUT);
  pinMode(setmpu_pin, INPUT);

  //setup accesspoint 

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();

  // Set up MQTT client
  client.setServer(mqtt_broker, 1883);
  
  // Connect to MQTT broker
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Connected to MQTT");
      dp_oled("mqtt connect");
    } else {
      Serial.print("Failed to connect to MQTT. State: ");
      Serial.println(client.state());
      dp_oled("failed to connect mqtt");
      delay(2000);
    }
  }
  Serial2.write("O");

  mpu_setup();
}

void loop() {

  //reconnect mqtt
  if (!client.connected()) {
    while (!client.connected()) {
      Serial.println("Reconnecting to MQTT...");
      if (client.connect(mqtt_client_id)) {
        Serial.println("Reconnected to MQTT");
        dp_oled("Reconnect mqtt");
      } else {
        Serial.print("Failed to reconnect to MQTT. State: ");
        dp_oled("failed to connect mqtt");
        Serial.println(client.state());
        delay(2000);
      }
    }
  }

  //measure button 
  if (digitalRead(measure_pin) == LOW) {
    laser_measure();
    laser_measure1();
    delay(500);
  }

  //reset button
  if (digitalRead(reset_pin) == LOW) {
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);

    if (client.publish("reset", payload)) {
      dp_oled("reset"); 
    }
    client.loop();
    delay(500);
  }

  //next button
  if (digitalRead(next_pin) == LOW) {
    //your command
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);

    if (client.publish("next", payload)) {
      dp_oled("next"); 
    } else {
      dp_oled("fail"); 
    }
    // Loop the client
    client.loop();
    delay(500);
  }

  //set mpu and start measure sensor button
  if (digitalRead(setmpu_pin) == LOW) {
    //your command
    dp_oled("setup mpu");
    mpu_setup();
    Serial2.write("D");
    delay(500);
  }
}

void mpu_setup() {
// join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  Wire.setClock(400000);  // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif
  Serial.begin(115200);
  while (!Serial)
    ;  // wait for Leonardo enumeration, others continue immediately
  //Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();

  //Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788);  // 1688 factory default for my test chip

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    // turn on the DMP, now that it's ready
    mpu.setDMPEnabled(true);
    mpuIntStatus = mpu.getIntStatus();
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
}

void dp_oled(String msg ) {
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
  delay(500);
  // collect output from serialport 2 in string
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
  client.loop();
  dp_oled(stringOne);
  stringOne = "";
}

void mpu_measure() {
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {  // Get the Latest packet
#ifdef OUTPUT_READABLE_YAWPITCHROLL
    // display Euler angles in degrees
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    rotation = ypr[0] * 180 / M_PI;
    facing_up = ypr[2] * 180 / M_PI;
#endif
    delay(500);
  }
}