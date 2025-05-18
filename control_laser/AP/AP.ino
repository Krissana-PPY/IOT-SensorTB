#include <WiFi.h>

// Define the SSID and password for the Access Point
const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";

void setup() {
  Serial.begin(115200);
  // Start the Wi-Fi Access Point
  WiFi.softAP(ssid, password);

  // Print the IP address of the Access Point
  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // ...existing code...
}