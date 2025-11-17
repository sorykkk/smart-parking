#ifndef CONFIG_H
#define CONFIG_H

// ==================== WiFi Configuration ============================ //
// Include for WIFI_SSID and WIFI_PASS
#include "env.h"

// ==================== Backend HTTP Configuration ============================ //
// Backend server settings for device registration

#define BACKEND_REGISTER_URL     "api/device/register"

// ==================== Device Configuration ============================ //
#define DEVICE_PREFIX    "esp32_dev"
#define DEVICE_LOCATION  "Complexul Studentesc P1"
#define DEVICE_LATITUDE  45.74956539097931  // Location Timisoara, Romania
#define DEVICE_LONGITUDE 21.240075184660427

// ==================== Sensor Configuration ============================ //
// Distance sensor settings
#define DISTANCE_MIN_CM 5
#define DISTANCE_MAX_CM 50 // Distance below this means occupied

// Timing settings
#define SENSOR_READ_INTERVAL 1000  // Read sensors every 10 seconds (in milliseconds)

// ==================== Camera Configuration ============================ //
// TODO: Camera module will be added in future iterations
// #define CAMERA_QUALITY 10  // 0-63 lower means higher quality

#endif


