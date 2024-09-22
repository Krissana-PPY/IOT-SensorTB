#define RXD2 16  // Adjust as per your wiring
#define TXD2 17  // Adjust as per your wiring

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  Serial.println("ESP32 Ready to communicate...");

  // Start hardware serial communication (Serial2)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  // Send 'F' to the Arduino UNO
  Serial2.write('F');
  delay(1000); // Wait 1 second
  Serial2.write('S');
  delay(1000); // Wait 1 second
  Serial2.write('B');
  delay(1000); // Wait 1 second
  Serial2.write('S');
  delay(1000); // Wait 1 second
}