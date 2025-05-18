// WiFi STA mode settings   
const char* ssid_STA = "ESP32-Access-Point";
const char* password_STA = "123456789";

// MQTT settings
const char* mqtt_broker = "192.168.4.100";
const char* mqtt_client_id = "esp32-sensor";


// MQTT topics
// Robot Box
const char* forward_topic = "F";
const char* back_topic = "B";
const char* lift_topic = "L35";
const char* down_topic = "D35";
const char* done_topic = "done";
// Robot Box
const char* twofloors_topic = "2F";
const char* threefloors_topic = "3F";
const char* fourfloors_topic = "4F";
const char* UDFfloors_topic = "UDF";
const char* test_topic = "test";
const char* error_topic = "error";

const char* mqtt_topic = "measure";
const char* finish_topic = "finish";
const char* NoProducts_topic = "NoProducts";      