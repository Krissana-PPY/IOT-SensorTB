#include <WiFi.h>

// Replace with your network credentials
//const char* ssid = "kku-wifi@robot";
//const char* password = "1q2w3e4r5t@robot";

// Access Point credentials
const char* ap_ssid = "ESP32-AccessPoint";
const char* ap_password = "123456789";

//#define IP_PROTO_TCP 6
//#define IP_PROTO_UDP 17

// Callback function for enabling NAT
/*void enableNAT(void* arg) {
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
}*/

void onNewClientConnected(arduino_event_id_t event, arduino_event_info_t info) {
  Serial.print("New device connected: ");
  Serial.print("MAC: ");
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1], info.wifi_ap_staconnected.mac[2],
           info.wifi_ap_staconnected.mac[3], info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
  Serial.println(macStr);
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi network in Station mode
  /*Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  // Print Station IP Address
  Serial.println("Connected to WiFi");
  Serial.print("STA IP Address: ");
  Serial.println(WiFi.localIP());*/

  // Configure Access Point
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Access Point Started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Register event handler for new client connections
  WiFi.onEvent(onNewClientConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

  // Use tcpip_callback to safely enable NAT
  //tcpip_callback(enableNAT, NULL);
}

void loop() {
  // Nothing to do here
}
