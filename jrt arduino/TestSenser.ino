#include <Arduino.h>

#define RXD2 16  // Serial2 RX pin
#define TXD2 17  // Serial2 TX pin

String stringOne;

void setup() {
  Serial.begin(115200);                          
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);  

  delay(100);
  Serial.flush();

  Serial.println("Setup complete. Waiting for measurements...");
}

void loop() {
  laser_measure();
  delay(1000);  // Delay 1 second between measurements
}

void laser_measure() {
  Serial2.write("D");  //Study the commands at the Datasheet.
  delay(500);          


  while (Serial2.available()) {
    char data = Serial2.read();
    stringOne += data;
  }



  Serial.print("Raw data bytes: ");
  for (size_t i = 0; i < stringOne.length(); i++) {
    Serial.print((byte)stringOne[i], HEX);
    Serial.print(" ");
  }
  Serial.println();


  Serial.print("Received data length: ");
  Serial.println(stringOne.length());

  Serial.print("Received data: ");
  Serial.println(stringOne);


  stringOne = "";
}
