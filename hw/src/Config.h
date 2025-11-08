#ifndef CONFIG_H
#define CONFIG_H

// include for:
//  * MQTT_PASS
//  * WIFI_SSID
//  * WIFI_PASS 
#include "env.h"

// MQTT Broker settings (Raspberry Pi server)
//TODO: make it also retrievable from an external storage like database, or use backend api to retrieve
//TODO: for now it's ok to be hardcoded
#define MQTT_BROKER "192.168.1.103"
#define MQTT_PORT 1883
#define MQTT_USER_PREFIX "esp32_dev"

// MQTT Topics
#define MQTT_TOPIC_SENSORS "sensors/"
// #define MQTT_TOPIC_CONTROL "control/"
#define MQTT_TOPIC_REGISTER_DEVICE "device/register/"
#define MQTT_TOPIC_REGISTER_SENSORS "sensors/register/"

// ====================== ESP32 & sensors configuration ====================== //

// Device registration settings
#define DEVICE_LOCATION "Complexul Studentesc P1"
#define DEVICE_LATITUDE 45.74956539097931  // Location Timisoara, Romania
#define DEVICE_LONGITUDE 21.240075184660427


// Sensor threshold settings
#define DISTANCE_MIN_CM 5
#define DISTANCE_MAX_CM 50 // Distance below this means occupied

// Camera settings
//TODO: camera module will be deactivated for now
// #define CAMERA_QUALITY 10  // 0-63 lower means higher quality

// Timing settings
#define DISTANCE_SENSOR_READ_INTERVAL 10000  // Read sensors every 10 seconds
//TODO: Sensor will send only if it's occupied state will change

#endif

