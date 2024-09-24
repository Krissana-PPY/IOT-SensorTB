// WiFi and MQTT settings
const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";
const char* mqtt_broker = "192.168.4.100";
const char* mqtt_client_id = "esp32-sensor";
const char* mqtt_topic = "measure";
const char* finish_topic = "finish";

// MQTT topics
const char* reverse_topic = "reverse";
//const char* forward_topic = "forward";
//////////////////Robot Box////////////////////
const char* forward_topic = "F";
const char* back_topic = "B";
const char* start_topic = "start";
//////////////////Robot Box////////////////////
const char* twofloors_topic = "2F";
const char* threefloors_topic = "3F";
const char* fourfloors_topic = "4F";
const char* UDFfloors_topic = "UDF";
const char* test_topic = "test";
const char* ERROR_TOPIC = "error";