#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_eap_client.h>  // ใช้ไฟล์ใหม่แทน esp_wpa2.h
#include <lwip/lwip_napt.h>
#include <lwip/tcpip.h>

// Enterprise WiFi credentials
const char* ssid = "Your_Enterprise_SSID";
const char* username = "Your_Username";
const char* password = "Your_Password";

// Access Point credentials  
const char* ap_ssid = "ESP32-Access-Point";
const char* ap_password = "123456789";

#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

void enableNAT(void* arg) {
  ip_napt_enable(WiFi.softAPIP(), 1);
  Serial.println("NAT Routing enabled");

  ip4_addr_t ap_ip;
  ip4_addr_t sta_ip;
  ip4addr_aton(WiFi.softAPIP().toString().c_str(), &ap_ip);
  ip4addr_aton(WiFi.localIP().toString().c_str(), &sta_ip);

  u32_t ap_ip_u32 = ip4_addr_get_u32(&ap_ip);
  u32_t sta_ip_u32 = ip4_addr_get_u32(&sta_ip);

  ip_portmap_add(IP_PROTO_TCP, ap_ip_u32, 0, sta_ip_u32, 0);
  ip_portmap_add(IP_PROTO_UDP, ap_ip_u32, 0, sta_ip_u32, 0);

  Serial.println("NAT rules configured");
}

void setup() {
  Serial.begin(115200);


  // Configure WiFi for WPA2 Enterprise
  WiFi.mode(WIFI_STA);
  
  // Configure WPA2 Enterprise using new API
  esp_eap_client_set_identity((uint8_t *)username, strlen(username));
  esp_eap_client_set_username((uint8_t *)username, strlen(username));
  esp_eap_client_set_password((uint8_t *)password, strlen(password));
  esp_wifi_sta_enterprise_enable();

  // Connect to Enterprise WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to Enterprise WiFi");
    return;
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("STA IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure Access Point
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Access Point Started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  tcpip_callback(enableNAT, NULL);
}

void loop() {
  // Check connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, trying to reconnect...");
    esp_wifi_sta_enterprise_enable();
    WiFi.begin(ssid);
  }
  delay(10000);
}