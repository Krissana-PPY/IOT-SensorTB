#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#define OUTPUT_READABLE_YAWPITCHROLL

MPU6050 mpu;

bool dmpReady = false;   // set true if DMP init was successful
uint8_t mpuIntStatus;    // holds actual interrupt status byte from MPU
uint8_t devStatus;       // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;     // expected DMP packet size (default is 42 bytes)
uint8_t fifoBuffer[64];  // FIFO storage buffer

Quaternion q;         // [w, x, y, z]         quaternion container
VectorFloat gravity;  // [x, y, z]            gravity vector
float ypr[3];         // [yaw, pitch, roll]   yaw/pitch/roll container

float rotation;
float facing_up;

volatile bool mpuInterrupt = false;  // indicates whether MPU interrupt pin has gone high

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);  // 400kHz I2C clock

  // Initialize MPU6050
  mpu.initialize();
  devStatus = mpu.dmpInitialize();

  // Set your gyro offsets here
  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);
  mpu.setZAccelOffset(1788);  // Adjust based on your setup

  // Check if DMP initialization was successful
  if (devStatus == 0) {
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.setDMPEnabled(true);
    mpuIntStatus = mpu.getIntStatus();
    packetSize = mpu.dmpGetFIFOPacketSize();
    dmpReady = true;
  } else {
    Serial.println("DMP Initialization failed!");
  }
}

void loop() {
  if (!dmpReady) return;

  // Check if new data is available
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    // Get yaw, pitch, roll data
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    
    rotation = ypr[0] * 180 / M_PI;  // Yaw
    facing_up = ypr[2] * 180 / M_PI;  // Roll
    
    Serial.print("Rotation (Yaw): ");
    Serial.print(rotation);
    Serial.print(" | Facing Up (Roll): ");
    Serial.println(facing_up);
  }
}
