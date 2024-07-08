#define PUL_PIN 25 // พินสัญญาณ Pulse
#define DIR_PIN 26 // พินสัญญาณทิศทาง
#define ENA_PIN 27 // พินสัญญาณ Enable

void setup() {
  Serial.begin(9600);
  pinMode(PUL_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  digitalWrite(ENA_PIN, LOW); // Enable the driver
  delay(1000); // รอให้ระบบพร้อม
}

void loop() {
  // เปิดใช้งานไดร์เวอร์ TB6600
  digitalWrite(ENA_PIN, LOW); // หรือ HIGH ขึ้นอยู่กับการเชื่อมต่อของคุณ
  
  // หมุนมอเตอร์ไปทิศทางหนึ่ง 180°
  Serial.println("หมุน 180 องศา ");
  digitalWrite(DIR_PIN, HIGH); 
  stepMotor(3200); // หมุนมอเตอร์ 3200 microstep (180°) เนื่องจากตั้งความละเอียดเป็น 1/16
  delay(1000); // หยุดพัก 1 วินาที
  
  // หมุนมอเตอร์กลับทิศทางเดิม 90°
  Serial.println("หมุนกลับ 90 องศา");
  digitalWrite(DIR_PIN, LOW); 
  stepMotor(1600); // หมุนมอเตอร์ 1600 microstep (90°)
  delay(1000); // หยุดพัก 1 วินาที
  
  // ปิดการใช้งานไดร์เวอร์ TB6600
  digitalWrite(ENA_PIN, HIGH); // หรือ LOW ขึ้นอยู่กับการเชื่อมต่อของคุณ
}

void stepMotor(int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(PUL_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(PUL_PIN, LOW);
    delayMicroseconds(500);
  }
}
