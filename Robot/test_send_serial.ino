#define RXD1 14  // Adjust as per your wiring7 9tx
#define TXD1 13  // Adjust as per your wiring5 8rx
void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  Serial.println("ESP32 Ready to communicate...");
  // Start hardware serial communication (Serial2)
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);
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
void loop() {
    Serial1.write('F');
//    delay(3000);
    Serial.print("send F");
    if (!check_done(10000)) return;  // หาก check_done ใช้เวลานานเกิน 10 วิให้จบการทำงาน
    Serial1.write('F');
//    delay(3000);
    Serial.print("send F");
    if (!check_done(10000)) return;
    Serial1.write('B');
//    delay(3000);
    Serial.print("send B");
    if (!check_done(10000)) return;
    Serial1.write('B');
//    delay(3000);
    Serial.print("send B");
    if (!check_done(10000)) return;
}