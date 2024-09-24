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

// Quaternion and vector variables
Quaternion q;
VectorFloat gravity;
float ypr[3];

// Define devStatus, mpuIntStatus, and packetSize
uint8_t devStatus;
uint8_t mpuIntStatus;
uint16_t packetSize;

#define DEBUG 1

// Stepper motor settings
#define PUL_PIN 26  // Pulse pin
#define DIR_PIN 27  // Direction pin
#define ENA_PIN 25  // Enable pin

#define RXD2 16
#define TXD2 17

#define RXD1 14  // Adjust as per your wiring
#define TXD1 13  // Adjust as per your wiring

#define STEP_ANGLE 0.1125 // Angle per step in degrees

uint8_t RETRY = 0; 

enum motion {OPEN = 1 , MEASURE, STATE, CLOSE};
enum MPU  {rotation, facing_up};

WiFiClient espClient;
PubSubClient client(espClient);

/* Function Prototypes */
void reconnect_mqtt();
void mpu_setup();
void twoflools();
void threefloors();
void UDFfloors();
void stepMotorconvert(int steps);
void control_stepper_motor(int step[], float distanceOfmotor);
void stepMotorWithLaserMeasurement(int steps);
void mpu_measure(float measure_return[]);
/*------------------------------*/


void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
 
  Wire.begin();
  Wire.setClock(400000);
  WiFi.softAP(ssid, password);
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);
  reconnect_mqtt();
  Serial2.write("O");
  mpu_setup(); 

  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable the driver
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Connected to MQTT");
      client.subscribe(reverse_topic);
      client.subscribe(forward_topic);
      client.subscribe(twofloors_topic);
      client.subscribe(threefloors_topic);
      client.subscribe(fourfloors_topic);
      client.subscribe(UDFfloors_topic);
      client.subscribe(test_topic);
      client.subscribe(back_topic);
      client.subscribe(start_topic);
    } else {
      Serial.print("Failed to connect to MQTT. State: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void mpu_setup() {

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


void stepMotorconvert (int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);  delayMicroseconds(1000);
    digitalWrite(PUL_PIN, LOW);   delayMicroseconds(1000);
  }
}




void control_stepper_motor(int step[],  float distanceOfmotor) {
  if (distanceOfmotor > 0) {
    float angle_rad = atan(0.20 / (distanceOfmotor));
    float angle_rad_lasor = atan(0.06 / (distanceOfmotor));
    float angle_deg = angle_rad * 180 / M_PI;
    float angle_deg_lasor = angle_rad_lasor * 180 / M_PI;
    step[0] = floor(angle_deg / STEP_ANGLE); // Calculate the number of steps needed // steps 
    step[1] = floor(angle_deg_lasor / STEP_ANGLE); // Calculate the number of steps needed steps_lasor
  }
}

void stepMotorWithLaserMeasurement(int steps) {
  int delayPerStep = (6000000) / (2 * steps); // Delay per step in microseconds for 6 seconds total
  int stepslaser = (2000000) / (2 * delayPerStep); // 2 seconds in milliseconds
  int stepscheck = stepslaser;

  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(delayPerStep);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(delayPerStep);
    if (i == stepscheck) {
      if (laser_measure1() == true) 
      {

      } else {

      }
      stepscheck += stepslaser;
    }
  }
}

void twoflools()
 {
  int steps[2] = {0, 0}; // steps, step_lasor
  digitalWrite(ENA_PIN, LOW);
  mpu_setup();
  delay(500);
  float distanceOfmotor = laser_value(MEASURE);
  if (distanceOfmotor > 0) {
    control_stepper_motor(steps, distanceOfmotor);
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert(steps[0] * 2);  delay(500);
//    digitalWrite(DIR_PIN, HIGH); 
    stepMotorWithLaserMeasurement(steps[1]);  delay(500);
    client.publish(finish_topic,"");   delay(500);
    digitalWrite(DIR_PIN, HIGH); 
    stepMotorconvert(steps[0] * 2);  delay(500);
    stepMotorWithLaserMeasurement(steps[1]);   delay(500);
    client.publish(finish_topic,"");
//    digitalWrite(DIR_PIN, LOW);
//    stepMotorconvert(steps[1]);   delay(500);
//    client.publish(forward_topic,"");
  } else {
    client.publish(ERROR_TOPIC,"twoflools");
  }   
}


void threefloors()
 {
  int steps[2] = {0, 0}; // steps, step_lasor
  digitalWrite(ENA_PIN, LOW);
  mpu_setup();   delay(500);
  float distanceOfmotor = laser_value(MEASURE);
  if(distanceOfmotor > 0) {
    control_stepper_motor(steps, distanceOfmotor);
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert(steps[0] * 2);     delay(500);
//    digitalWrite(DIR_PIN, HIGH);
    stepMotorWithLaserMeasurement(steps[1]); delay(500);
    client.publish(finish_topic,"");     delay(500);
    digitalWrite(DIR_PIN, HIGH); 
    stepMotorconvert(steps[0] * 2);     delay(500);
    stepMotorWithLaserMeasurement(steps[1]);  delay(500);
    client.publish(finish_topic,"");     delay(500);
    stepMotorconvert((steps[0] * 2)); delay(500);
    digitalWrite(DIR_PIN, LOW);
    stepMotorWithLaserMeasurement(steps[1]); delay(500);
    client.publish(finish_topic,""); 
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert((steps[0] * 2) - steps[1]); delay(500);
//    client.publish(forward_topic,"");
  } else {
    client.publish(ERROR_TOPIC,"threeflools");
  }
}

void UDFfloors()
 {
  int steps[2] = {0, 0}; // steps, step_lasor
  digitalWrite(ENA_PIN, LOW);
  mpu_setup(); delay(500);
  float distanceOfmotor = laser_value(MEASURE);
  if(distanceOfmotor > 0) {
    control_stepper_motor(steps, distanceOfmotor);
    digitalWrite(DIR_PIN, HIGH);
    stepMotorconvert(steps[0]);  delay(500);
//    digitalWrite(DIR_PIN, LOW);
    stepMotorWithLaserMeasurement(steps[1]); delay(500);
    client.publish(finish_topic,"");
    digitalWrite(DIR_PIN, LOW);
    stepMotorconvert(steps[0] + steps[1]);      
    //client.publish(forward_topic,"");
  } else {
    client.publish(ERROR_TOPIC,"UDFfloors");
  }
}

void test()
 {
    float steps[2] = {0.0, 0.0}; // steps, step_lasor
    float distanceOfmotor = laser_value(MEASURE);
    mpu_measure(steps);
    Serial.println(distanceOfmotor);
    Serial.printf("rotation %.2f\n", steps[0]);
    Serial.printf("facing %.2f\n", steps[1]);

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
//    client.publish("reset", payload);

  } else if (String(topic) == twofloors_topic) {
    twoflools();
  
  } else if (String(topic) == threefloors_topic) {
    threefloors(); 

  } else if (String(topic) == UDFfloors_topic) {
    UDFfloors();

  } else if (String(topic) == test_topic) {
    test();

  } else if (String(topic) == start_topic) {
    twoflools();
    Serial1.write('F');
    if (!check_done(10000)) return;  // หาก check_done ใช้เวลานานเกิน 10 วิให้จบการทำงาน
    UDFfloors();
    Serial1.write('F');
    if (!check_done(10000)) return;
    twoflools();
    Serial1.write('B');
    if (!check_done(10000)) return;
    Serial1.write('B');
    if (!check_done(10000)) return;
    Serial1.write('B');
    if (!check_done(10000)) return;
  }
  
}

bool check_done(unsigned long timeout) {
  String received = ""; // ตัวแปรสำหรับเก็บข้อมูลที่อ่านได้
  unsigned long startMillis = millis(); // เก็บเวลาที่เริ่มต้น

  while (true) {
    // ตรวจสอบเวลาที่ผ่านไป หากเกิน timeout (10 วินาที = 10000 มิลลิวินาที) จะออกจากฟังก์ชันทันที
    if (millis() - startMillis >= timeout) {
      Serial.println("Timeout: Exiting check_done");
      return false; // จบการทำงานเนื่องจากเกินเวลา
    }

    // รอให้มีข้อมูลเข้ามา
    while (!Serial1.available()) {
      // รอข้อมูล
      if (millis() - startMillis >= timeout) {
        Serial.println("Timeout while waiting for data: Exiting check_done");
        return false; // จบการทำงานเนื่องจากเกินเวลา
      }
    }

    // อ่านข้อมูลหนึ่งตัวอักษร
    char c = Serial1.read();
    received += c; // เก็บตัวอักษรที่อ่านได้

    // ตรวจสอบว่าข้อมูลที่ได้รับมีคำว่า "done" หรือไม่
    if (received.endsWith("done")) {
      Serial.print("Received from Arduino: ");
      Serial.println(received); // พิมพ์ข้อมูลที่ได้รับ
      return true; // ข้อมูลครบ จบการทำงานปกติ
    }
  }
}

/*
command  1 = open, 2 = measure, 3 = state 4 = close 

return -1 error
return value OK

*/

float laser_value(int Command)
{
  float value = -100; 
  for(int i = 1; i <= 3; i++)
  {
    value = laser_sensor_function(Command); 
    if(value != 0 || value != -1)
    {
       return value;    
    }else
    {
       delay(1000); // delay 1 second before checksing sensor state 
       float state =   laser_sensor_function(STATE);
      // Serial.println("retry");
       if(state > 0) 
       {
        delay(1000); // delay 1 second before retesting  
        continue;
       }
       else {
        client.publish(ERROR_TOPIC,"Sensor Error");  
       } 
    }
  }
  return value; 
}


//เปลี่ยนเป็น serial ที่ส่งค่า 

float laser_sensor_function(int Command)
{
 
  String stringOne;
  float return_value = -100; 
  Serial.flush();
  
  switch (Command)
  {
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
    
  }// swith-case 

  uint32_t now = millis();
//  Serial.println(stringOne);
  while (stringOne.length() < 20) {
    if(Serial2.available())
    { 
      char data = Serial2.read();
      stringOne += data;
      // stringOne += Serial.read(); 
    }
    // safety for array limit && timeout... in  seconds...
    if (millis() - now > 2000) {
      Serial.println("break");
      break;

    }
   }


   if (stringOne.startsWith(":Er")) { // error 
      return_value =  -1.0;

    }else if(stringOne.indexOf("K") != -1 && (Command == OPEN || Command == CLOSE)) //assume OK message 
    {
      return_value = 100;  
    }
    else if(stringOne.indexOf("m")  && Command == MEASURE)
    {
      int start = stringOne.indexOf(":") + 1;
      int end = stringOne.indexOf("m");
      String distanceStr = stringOne.substring(start, end);
      return_value = distanceStr.toFloat();
    
    }else if(Command == STATE)
    {
      String distanceStr = stringOne.substring(1, 2);
      return_value = distanceStr.toFloat();
    }else {
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
    if(sensor_distance1 != 0)
    {
      StaticJsonDocument<200> doc;
      doc["rotation"] = abs(mpu_value[rotation]);
      doc["facing_up"] = abs(mpu_value[facing_up]);
      doc["distance"] = sensor_distance1;
      char payload[200];
      serializeJson(doc, payload);
      client.publish(mqtt_topic, payload);
      success = true; 
    }
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
//  laser_sensor_function(MEASURE);
//  delay(2000);
}
