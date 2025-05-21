#include <Arduino.h>

// Define RX and TX pins for Serial2
#define RXD2 16  // Serial2 RX pin
#define TXD2 17  // Serial2 TX pin

// Create a String to store data read from Serial2
String stringOne;

void setup() {
  Serial.begin(115200);                          // Start Serial communication at 115200 baud rate
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);  // Start Serial2 communication at 19200 baud rate

  delay(100);     // Wait for setup to complete
  Serial.flush(); // Clear Serial buffer

  Serial.println("Setup complete. Waiting for measurements..."); // Print setup complete message
}

void loop() {
  laser_measure();  // Call function to measure distance with laser
  delay(1000);      // Wait 1 second between each measurement
}

/*
The automatic measurement process is initiated, and the module
returns a string containing measurement distance and
measurement signal quality, such as: "12.345m,0079".
The measurement distance is expressed as 12.345M, signal quality 79.
*/

void laser_measure() {
  Serial2.write("D");  // Send command "D" to start measurement
  delay(500);          // Wait 500 ms for the module to measure

  // Read data from Serial2 if available
  while (Serial2.available()) {
    char data = Serial2.read();  // Read one character at a time
    stringOne += data;           // Append read data to stringOne
  }

  // Print raw data bytes in hexadecimal format
  Serial.print("Raw data bytes: ");
  for (size_t i = 0; i < stringOne.length(); i++) {
    Serial.print((byte)stringOne[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Print the length of received data
  Serial.print("Received data length: ");
  Serial.println(stringOne.length());

  // Print the received data as a string
  Serial.print("Received data: ");
  Serial.println(stringOne);

  // Clear stringOne for the next measurement
  stringOne = "";
}
