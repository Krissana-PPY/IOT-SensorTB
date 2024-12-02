#include <SoftwareSerial.h>

// Define RX and TX pins for SoftwareSerial
SoftwareSerial mySerial(7, 6); // RX, TX

void setup() {
  // Begin serial communication
  Serial.begin(9600);      // Serial monitor
  mySerial.begin(9600);    // Serial communication with ESP32

  Serial.println("Arduino Ready to communicate...");
}

void loop() {
  // Check if data is available from ESP32
  if (mySerial.available()) {
    char received = mySerial.read(); // Read the character
    Serial.print("Received from ESP32: ");
    Serial.println(received);        // Print it on the Serial Monitor

    // Respond with 'F'
    mySerial.write('F');
    Serial.println("Sent to ESP32: F");
  }

  delay(1000); // Wait for 1 second before repeating
}
