#include <Arduino.h>
#include <Wire.h> 
#include <PubSubClient.h>
#include <WiFi.h>
#include "laser_parameter.h"
#include <ArduinoJson.h>
#include <MPU6050_6Axis_MotionApps20.h>

// Declare the MPU6050 object
MPU6050 mpu;

// Buffer for DMP data
uint8_t fifoBuffer[64];

// Quaternion and vector variables for orientation and gravity
Quaternion q;
VectorFloat gravity;
float ypr[3];

// Define devStatus, mpuIntStatus, and packetSize
uint8_t devStatus;
uint8_t mpuIntStatus;
uint16_t packetSize;

#define DEBUG 1

// Stepper motor pin definitions
#define PUL_PIN 26  // Pulse pin
#define DIR_PIN 27  // Direction pin
#define ENA_PIN 25  // Enable pin

// Serial communication pins for laser sensor
#define RXD2 16
#define TXD2 17

// Stepper motor step angle in degrees
#define STEP_ANGLE 0.1125 

// Flag to check if the topic has been received
volatile bool doneReceived = false;  

uint8_t RETRY = 0; 

// Enumerations for motion and MPU states
enum motion {OPEN = 1 , MEASURE, STATE, CLOSE};
enum MPU  {rotation, facing_up};

// WiFi and MQTT client objects
WiFiClient espClient;
PubSubClient client(espClient);

/* Function Prototypes */
void reconnect_mqtt();
void mpu_setup();
void twofloors();
void threefloors();
void UDFfloors();
void step_motor_move(int steps);
void control_stepper_motor(int step[], float distanceOfmotor);
void step_motor_withLaser_measurement(int steps);
void start_step_laser(int steps, int Logic1, int Logic2);
void move_motor_start(int Logic);
void control_logic_motor(int steps, float distanceOfmotor);
void waitForTopic(const char* done_topic);
void test();
void callback(char* topic, byte* payload, unsigned int length);
float laser_value(int Command);
float laser_sensor_function(int Command);
bool laser_measure1();
void mpu_measure(float measure_return[]);
void prepare_floors(int steps[2], float &distanceOfmotor);
/*------------------------------*/

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
 
  // Initialize I2C communication
  Wire.begin();
  Wire.setClock(400000);

  // Connect to WiFi
  WiFi.begin(ssid_STA, password_STA);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up MQTT server and callback
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);
  reconnect_mqtt();

  // Initialize laser sensor
  Serial2.write("O");

  // Initialize MPU6050
  mpu_setup(); 

  // Configure stepper motor pins
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable the driver
}

void reconnect_mqtt() {
  // Reconnect to MQTT broker if disconnected
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Connected to MQTT");
      // Subscribe to necessary topics
      client.subscribe(forward_topic);
      client.subscribe(twofloors_topic);
      client.subscribe(threefloors_topic);
      client.subscribe(fourfloors_topic);
      client.subscribe(UDFfloors_topic);
      client.subscribe(test_topic);
      client.subscribe(back_topic);
      client.subscribe(done_topic);
    } else {
      Serial.print("Failed to connect to MQTT. State: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

/**
 * @brief Initialize MPU6050 and configure DMP
 */
void mpu_setup() {
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  // Set gyro and accelerometer offsets
  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788);

  if (devStatus == 0) {
    // Calibrate and enable DMP
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    mpu.setDMPEnabled(true);
    mpuIntStatus = mpu.getIntStatus();
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
}

/**
 * @brief Move the stepper motor by the specified number of steps
 * @param steps Number of steps to move
 */
void step_motor_move(int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);  
    delayMicroseconds(1000);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(1000);
  }
}

/**
 * @brief Calculate the number of steps required based on the distance
 * @param step Array to store calculated steps [motor, laser]
 * @param distanceOfmotor Distance for calculation
 * 1 พาเลท สูง 1.6 เมตร
 */
void control_stepper_motor(int step[], float distanceOfmotor) {
  if (distanceOfmotor > 0) {
    float angle_rad = atan(0.80 / distanceOfmotor);
    float angle_rad_lasor = atan(0.40 / distanceOfmotor);
    float angle_deg = angle_rad * 180 / M_PI;
    float angle_deg_lasor = angle_rad_lasor * 180 / M_PI;
    step[0] = floor(angle_deg / STEP_ANGLE);      // Steps for motor
    step[1] = floor(angle_deg_lasor / STEP_ANGLE); // Steps for laser
  }
}

/**
 * @brief Move the motor in steps while performing laser measurements
 * @param steps Total steps to move
 */
void step_motor_withLaser_measurement(int steps) {
  float stepsPerLoop = static_cast<float>(steps) / 3.0f; 
  int stepsAccumulated = 0;
  for (int i = 0; i < 3; i++) {
    int stepsThisIteration = static_cast<int>(stepsPerLoop); 
    stepsAccumulated += stepsThisIteration;
    if (i == 2) {
      stepsThisIteration += steps - stepsAccumulated;
    }
    for (int j = 0; j < stepsThisIteration; j++) {
      digitalWrite(PUL_PIN, HIGH); 
      delayMicroseconds(1000);
      digitalWrite(PUL_PIN, LOW);
      delayMicroseconds(1000);
    }

    // Perform laser measurement after each segment
    if (laser_measure1()) {
      // Measurement successful, handle if needed
    }
  }
}

void start_step_laser(int steps, int Logic1, int Logic2) {
  // Start the motor and laser measurement process
  digitalWrite(DIR_PIN, Logic1); 
  step_motor_withLaser_measurement(steps);  
  delay(250);
  digitalWrite(DIR_PIN, Logic2);
  step_motor_move(steps);  
  delay(250);
}
/** 
 *                           assume * 7.8m
 *                |---|---|---|---|---|---|---|---|---|---|
 *                |   |   |   |   | | |   |   |   |   |   |
 *                |   |   |   |   | | |   |   |   |   |   |
 * |sensor|*      |---|---|---|---|---|---|---|---|---|---|
 *          |     | * |   |   |   | | |   |   |   |   |   |
 *         1.6m   |   |   |   |   | * |   |   |   |   |   |
 *          |     |___|___|___|___|_|_|___|___|___|___|__*|
 * ----------------------------- 15m ----------------------
*/

void move_motor_start(int Logic) {
  // Move the motor to the starting position
  float degree = (atan(1.6 / 15)) * 180 / M_PI; // Desired angle for motor rotation
  int steps = floor(degree / STEP_ANGLE); // Calculate the number of steps needed
  digitalWrite(DIR_PIN, Logic);
  step_motor_move(steps);
}

void control_logic_motor(int steps, float distanceOfmotor) {
  // Control the motor logic based on the distance
  if (distanceOfmotor < 7.80) {
    start_step_laser(steps, LOW, HIGH);
  } else {
    start_step_laser(steps, HIGH, LOW);
  }
}

void waitForTopic(const char* done_topic) {
  // Wait for the specified topic to be received
  doneReceived = false; // Reset the flag
  while (!doneReceived) {
    client.loop();
    delay(100); // Small delay to avoid busy-waiting
  }
}

/**
 * @brief Helper function to prepare stepper motor and calculate steps/distance for floors
 * @param steps Output array for steps [motor, laser]
 * @param distanceOfmotor Output for calculated distance
 */
void prepare_floors(int steps[2], float &distanceOfmotor) {
  digitalWrite(ENA_PIN, LOW);
  mpu_setup();
  delay(500);
  move_motor_start(LOW);
  delay(500);

  while (steps[1] > 100 || steps[1] <= 0) {
    float distanceOfstart = laser_value(MEASURE);
    distanceOfmotor = distanceOfstart * cos(atan(1.6 / 15));
    control_stepper_motor(steps, distanceOfmotor);
    delay(500);
  }
  if (distanceOfmotor >= 14.40) {
    control_logic_motor(steps[1], distanceOfmotor);
    client.publish(NoProducts_topic, "No products");
    delay(250);
    client.publish(finish_topic, "No products completed.");
    delay(250);
    move_motor_start(HIGH);
    client.publish(forward_topic, "Forward");
    return;
  }
}

/**
 * @brief Process and control logic for 2 floors
 */
void twofloors() {
  int steps[2] = {0, 0};
  float distanceOfmotor = 0;
  prepare_floors(steps, distanceOfmotor);

  if (distanceOfmotor > 0 && distanceOfmotor < 14.40) {
    control_logic_motor(steps[1], distanceOfmotor);
    move_motor_start(HIGH);
    client.publish(finish_topic, "1 Floors completed.");
    delay(250);

    // Repeat measurement and movement for the next floor
    digitalWrite(DIR_PIN, HIGH);
    step_motor_move(steps[1]);
    start_step_laser(steps[1], HIGH, LOW);

    digitalWrite(DIR_PIN, LOW);
    step_motor_move(steps[1]);
    client.publish(finish_topic, "2 Floors completed.");
    delay(250);
    client.publish(forward_topic, "Forward");
    return;
  }

  // If none of the above, report an error
  client.publish(error_topic, "Twofloors");
}

/**
 * @brief Process and control logic for 3 floors
 */
void threefloors() {
  int steps[2] = {0, 0};
  float distanceOfmotor = 0;
  prepare_floors(steps, distanceOfmotor);

  if (distanceOfmotor > 0 && distanceOfmotor < 14.40) {
    control_logic_motor(steps[1], distanceOfmotor);
    move_motor_start(HIGH);
    client.publish(finish_topic, "1 Floors completed.");
    delay(250);

    digitalWrite(DIR_PIN, HIGH);
    step_motor_move(steps[1]);
    start_step_laser(steps[1], HIGH, LOW);

    digitalWrite(DIR_PIN, LOW);
    step_motor_move(steps[1]);
    //client.publish(lift_topic, "UP");
    client.publish(finish_topic, "2 Floors completed.");
 
    //waitForTopic(done_topic);

    // 3rd floor
    digitalWrite(DIR_PIN, HIGH);
    step_motor_move((steps[0] * 2) + steps[1]);
    start_step_laser(steps[1], HIGH, LOW);

    digitalWrite(DIR_PIN, LOW);
    step_motor_move((steps[0] * 2) + steps[1]);
    //client.publish(down_topic, "Down");
    client.publish(finish_topic, "3 Floors completed.");
    //waitForTopic(done_topic);
    client.publish(forward_topic, "Forward");
    return;
  }

  client.publish(error_topic, "Threefloors");
}

/**
 * @brief Process and control logic for highest floor
 */
void UDFfloors() {
  int steps[2] = {0, 0}; // steps[0]: motor, steps[1]: laser
  digitalWrite(ENA_PIN, LOW);
  mpu_setup();
  delay(500);
  client.publish(lift_topic, "UP");
  waitForTopic(done_topic);

  float distanceOfmotor = 0;
  while (distanceOfmotor <= 1.00) {
    distanceOfmotor = laser_value(MEASURE);
  }

  if (distanceOfmotor > 1.00) {
    digitalWrite(DIR_PIN, HIGH);
    step_motor_move(steps[0] * 2);
    start_step_laser(steps[1], HIGH, LOW);

    digitalWrite(DIR_PIN, LOW);
    step_motor_move(steps[0] * 2);
    client.publish(down_topic, "Down");
    client.publish(finish_topic, "UD Floors completed.");
    waitForTopic(done_topic);
    client.publish(forward_topic, "Forward");
    return;
  }

  client.publish(error_topic, "UDFfloors");
}

void test() {
    int steps[2] = {0, 0}; // steps, step_lasor
    mpu_setup(); delay(500);
    float distanceOfmotor = laser_value(MEASURE);
    if(distanceOfmotor > 0) {
      Serial.println(distanceOfmotor);
      laser_measure1();
      client.publish(finish_topic,"Test");
      Serial2.write("O");
    } else {
    client.publish(error_topic,"Test");
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (String(topic) == twofloors_topic) { 
    twofloors();
  } else if (String(topic) == threefloors_topic) { 
    threefloors(); 
  } else if (String(topic) == UDFfloors_topic) { 
    UDFfloors();
  } else if (String(topic) == back_topic) {
    StaticJsonDocument<200> doc;
    doc["rotation"] = "-1";
    doc["facing_up"] = "-1";
    doc["distance"] = "-1";
    char payload[200];
    serializeJson(doc, payload);
  } else if (String(topic) == test_topic) {
    test();
  } else if (String(topic) == done_topic) {
    doneReceived = true;
  }
}
/*
command  1 = open, 2 = measure, 3 = state 4 = close 

return -1 error
return value OK

*/

float laser_value(int Command) {
  float value = -100; 
  for(int i = 1; i <= 3; i++) {
    value = laser_sensor_function(Command); 
    if(value != 0 || value != -1) {
       return value;    
    } else {
       delay(1000); // delay 1 second before checksing sensor state 
       float state =   laser_sensor_function(STATE);
       if(state > 0) {
        delay(1000); // delay 1 second before retesting  
        continue;
       }
       else {
        client.publish(error_topic,"Sensor Error");  
       } 
    }
  }
  return value; 
}
 
float laser_sensor_function(int Command) {
 
  String stringOne;
  float return_value = -100; 
  Serial.flush();
  
  switch (Command) {
    case OPEN:
      Serial2.write("O");
      break;
    case MEASURE:
      Serial2.write("D");
      break;
    case STATE:
      Serial2.write("S");
      break;
    case CLOSE:
      Serial2.write("C");
      break;
  }

  uint32_t now = millis();
  while (stringOne.length() < 20) {
    if(Serial2.available()) { 
      char data = Serial2.read();
      stringOne += data;
    }
    // safety for array limit && timeout... in  seconds...
    if (millis() - now > 2000) {
      Serial.println("break");
      break;
    }
  }
  if (stringOne.startsWith(":Er")) { // error 
    return_value =  -1.0;
  } else if (stringOne.indexOf("K") != -1 && (Command == OPEN || Command == CLOSE)) { //assume OK message 
    return_value = 100;  
  } else if (stringOne.indexOf("m")  && Command == MEASURE) {
    int start = stringOne.indexOf(":") + 1;
    int end = stringOne.indexOf("m");
    String distanceStr = stringOne.substring(start, end);
    return_value = distanceStr.toFloat();
  } else if (Command == STATE) {
    String distanceStr = stringOne.substring(1, 2);
    return_value = distanceStr.toFloat();
  } else {
    return_value = 0; //should not return this one. 
  }
  
  Serial.print("value : ");
  Serial.println(return_value,3);
  return return_value; 
}

bool laser_measure1() {
    bool success = false; 
    float mpu_value[2] = {0.0, 0.0};   
    mpu_measure(mpu_value);

    float sensor_distance1 = laser_value(MEASURE);
    if(sensor_distance1 != 0) {
      StaticJsonDocument<200> doc;
      doc["rotation"] = abs(mpu_value[rotation]);
      doc["facing_up"] = abs(mpu_value[facing_up]);
      doc["distance"] = sensor_distance1;
      char payload[200];
      serializeJson(doc, payload);
      client.publish(mqtt_topic, payload);
      success = true; 
    }
    Serial2.write("O");
    return success; 
}

  //ลบ comment ออก 
//  float  rotation =  measure_return[0];
//  float  facing_up =  measure_return[1];

void mpu_measure(float measure_return[]) {
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    measure_return[rotation] = ypr[0] * 180 / M_PI;
    measure_return[facing_up] = ypr[2] * 180 / M_PI;
    delay(500);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
}