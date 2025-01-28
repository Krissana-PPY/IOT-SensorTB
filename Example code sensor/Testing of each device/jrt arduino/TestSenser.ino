#include <Arduino.h>

// กำหนดขา RX และ TX สำหรับ Serial2
#define RXD2 16  // Serial2 RX pin
#define TXD2 17  // Serial2 TX pin

// สร้าง String เพื่อเก็บข้อมูลที่อ่านได้จาก Serial2
String stringOne;

void setup() {
  Serial.begin(115200);                          // เริ่มการสื่อสารผ่าน Serial ที่ baud rate 115200
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);  // เริ่มการสื่อสารผ่าน Serial2 ที่ baud rate 19200

  delay(100);     // รอให้การตั้งค่าเสร็จสมบูรณ์
  Serial.flush(); // ล้าง buffer ของ Serial

  Serial.println("Setup complete. Waiting for measurements..."); // แสดงข้อความว่าการตั้งค่าเสร็จสมบูรณ์
}

void loop() {
  laser_measure();  // เรียกฟังก์ชันวัดระยะทางด้วยเลเซอร์
  delay(1000);      // รอ 1 วินาทีระหว่างการวัดแต่ละครั้ง
}

/*The automatic measurement process is initiated, and the module
returns a string containing measurement distance and
measurement signal quality, such as："12.345m,0079"，The
measurement distance is expressed as12.345M ， Signal
quality79。
*/

void laser_measure() {
  Serial2.write("D");  // ส่งคำสั่ง "D" เพื่อเริ่มการวัด
  delay(500);          // รอ 500 มิลลิวินาทีเพื่อให้โมดูลทำการวัด

  // อ่านข้อมูลจาก Serial2 หากมีข้อมูล
  while (Serial2.available()) {
    char data = Serial2.read();  // อ่านข้อมูลทีละตัวอักษร
    stringOne += data;           // เพิ่มข้อมูลที่อ่านได้ลงใน stringOne
  }

  // แสดงข้อมูลดิบเป็นค่าไบต์ในรูปแบบเลขฐานสิบหก
  Serial.print("Raw data bytes: ");
  for (size_t i = 0; i < stringOne.length(); i++) {
    Serial.print((byte)stringOne[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // แสดงความยาวของข้อมูลที่ได้รับ
  Serial.print("Received data length: ");
  Serial.println(stringOne.length());

  // แสดงข้อมูลที่ได้รับ
  Serial.print("Received data: ");
  Serial.println(stringOne);

  // ล้างข้อมูลใน stringOne เพื่อเตรียมรับข้อมูลใหม่ในรอบถัดไป
  stringOne = "";
}
