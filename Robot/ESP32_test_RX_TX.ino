// Define the pins for RX and TX
#define RXD2 12  // Adjust as per your wiring
#define TXD2 13  // Adjust as per your wiring

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  Serial.println("ESP32 Ready to communicate...");

  // Start hardware serial communication (Serial2)
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  // Send 'S' to the Arduino UNO
  Serial2.write('S');
  Serial.println("Sent to Arduino: S");

  // Wait for a response from Arduino
  if (Serial2.available()) {
    char received = Serial2.read(); // Read the character
    Serial.print("Received from Arduino: ");
    Serial.println(received);       // Print it on the Serial Monitor
  }

  delay(1000); // Wait 1 second before sending 'S' again
  Serial2.write('B');
}