#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <lwip/lwip_napt.h>
#include <lwip/tcpip.h> // รวมไฟล์เพื่อใช้ tcpip_callback

// Replace with your network credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// Access Point credentials
const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "123456789";

// Define protocol constants
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

// Callback function for enabling NAT
void enableNAT(void* arg) {
  // Enable NAT
  ip_napt_enable(WiFi.softAPIP(), 1);
  Serial.println("NAT Routing enabled");

  // Set up IP forwarding
  ip4_addr_t ap_ip;
  ip4_addr_t sta_ip;
  ip4addr_aton(WiFi.softAPIP().toString().c_str(), &ap_ip);
  ip4addr_aton(WiFi.localIP().toString().c_str(), &sta_ip);

  u32_t ap_ip_u32 = ip4_addr_get_u32(&ap_ip);
  u32_t sta_ip_u32 = ip4_addr_get_u32(&sta_ip);

  // Add NAT rules
  ip_portmap_add(IP_PROTO_TCP, ap_ip_u32, 0, sta_ip_u32, 0);
  ip_portmap_add(IP_PROTO_UDP, ap_ip_u32, 0, sta_ip_u32, 0);

  Serial.println("NAT rules configured");
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi network in Station mode
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  // Print Station IP Address
  Serial.println("Connected to WiFi");
  Serial.print("STA IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure Access Point
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Access Point Started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Use tcpip_callback to safely enable NAT
  tcpip_callback(enableNAT, NULL);
}

void loop() {
  // Nothing to do here
}
