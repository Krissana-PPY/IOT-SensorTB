// WiFi STA (เชื่อมต่อ WiFi โรงงาน)
const char* ssid_STA = "kku-wifi@robot";
const char* password_STA = "1q2w3e4r5t@robot";

// WiFi AP (ปล่อย WiFi ให้ Pi4)
const char* ssid_AP = "ESP32-Access-Point";
const char* password_AP = "123456789";

// MQTT settings
const char* mqtt_broker = "192.168.4.100";
const char* mqtt_client_id = "esp32-sensor";


// MQTT topics
// Robot Box
const char* forward_topic = "F";
const char* back_topic = "B";
// Robot Box
const char* twofloors_topic = "2F";
const char* threefloors_topic = "3F";
const char* fourfloors_topic = "4F";
const char* UDFfloors_topic = "UDF";
const char* test_topic = "test";
const char* error_topic = "error";

const char* mqtt_topic = "measure";
const char* finish_topic = "finish";