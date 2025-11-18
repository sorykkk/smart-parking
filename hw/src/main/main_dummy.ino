// #include <Arduino.h>
// #include <vector>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include "esp_task_wdt.h"
// #include "../Config.h"
// #include "../WiFiManager.h"
// #include "../DistanceSensor.h"
// #include "../CameraDevice.h"
// #include "../MQTTClient.h"
// #include "../HttpClient.h"
// #include "time.h"

// using namespace FindSpot;

// // Watchdog timeout in seconds (increased to prevent premature resets)
// #define WDT_TIMEOUT 30

// // ==================== Dummy Sensor Class ============================ //
// /**
//  * Mock sensor that simulates distance readings without actual hardware
//  */
// class DummySensor : public ISensor {
// private:
//   Device& device;
//   String technology;
//   int index;
//   float currentDistance;
//   bool occupied;
//   int cycleCounter;
  
// public:
//   DummySensor(Device& dev, const String& tech, int idx) 
//     : device(dev), technology(tech), index(idx), currentDistance(100.0), 
//       occupied(false), cycleCounter(0) {}
  
//   void begin() override {
//     Serial.print("Dummy Sensor ");
//     Serial.print(index);
//     Serial.println(" initialized");
//   }
  
//   bool checkState() override {
//     // Simulate different parking patterns for each sensor
//     cycleCounter++;
    
//     switch (index) {
//       case 0:
//         // Sensor 0: Alternates every 3 readings (free -> occupied -> free)
//         if ((cycleCounter / 3) % 2 == 0) {
//           currentDistance = 60.0; // Free
//           occupied = false;
//         } else {
//           currentDistance = 25.0; // Occupied
//           occupied = true;
//         }
//         break;
        
//       case 1:
//         // Sensor 1: Alternates every 5 readings (slower changes)
//         if ((cycleCounter / 5) % 2 == 0) {
//           currentDistance = 70.0; // Free
//           occupied = false;
//         } else {
//           currentDistance = 15.0; // Occupied
//           occupied = true;
//         }
//         break;
        
//       case 2:
//         // Sensor 2: Random-like pattern using modulo
//         if ((cycleCounter % 7) < 4) {
//           currentDistance = 80.0; // Free
//           occupied = false;
//         } else {
//           currentDistance = 30.0; // Occupied
//           occupied = true;
//         }
//         break;
        
//       default:
//         // Additional sensors: simple alternation
//         if ((cycleCounter / 2) % 2 == 0) {
//           currentDistance = 65.0; // Free
//           occupied = false;
//         } else {
//           currentDistance = 20.0; // Occupied
//           occupied = true;
//         }
//         break;
//     }
    
//     Serial.print("    Distance: ");
//     Serial.print(currentDistance);
//     Serial.println(" cm");
    
//     return occupied;
//   }
  
//   String toJson() override {
//     StaticJsonDocument<256> doc;
//     doc["device_id"] = device.getId();
//     doc["sensor_index"] = index;
//     doc["technology"] = technology;
//     doc["occupied"] = occupied;
//     doc["distance_cm"] = currentDistance;
//     doc["timestamp"] = millis();
    
//     String jsonString;
//     serializeJson(doc, jsonString);
//     return jsonString;
//   }
  
//   // Manual control methods for testing
//   void setDistance(float distance) {
//     currentDistance = distance;
//     occupied = (distance < DISTANCE_MAX_CM);
//   }
  
//   void setOccupied(bool occ) {
//     occupied = occ;
//     currentDistance = occ ? 20.0 : 70.0;
//   }
// };

// // ==================== Global Objects ============================ //
// WiFiManager wifi;
// HttpClient httpClient;
// MQTTClient mqttClient;
// Device esp32device(DEVICE_PREFIX, DEVICE_LOCATION, DEVICE_LATITUDE, DEVICE_LONGITUDE);

// std::vector<ISensor*> sensors;
// std::vector<bool> sensorStateVector;

// unsigned long lastSensorRead = 0;

// // NTP server and timezone settings
// const char* ntpServer = "pool.ntp.org";
// const long  gmtOffset_sec = 7200;      // GMT+2
// const int   daylightOffset_sec = 3600; // Daylight saving

// // ==================== MQTT Callback ============================ //
// /**
//  * MQTT callback for incoming messages (optional - for future features)
//  */
// void mqttCallback(char* topic, byte* payload, unsigned int length) {
//   String message;
//   for (unsigned int i = 0; i < length; i++) {
//     message += (char)payload[i];
//   }
  
//   Serial.println("📨 MQTT Message on topic: " + String(topic));
//   Serial.println("📦 Payload: " + message);
// }

// // ==================== Setup ============================ //
// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   Serial.println("\n\n");
//   Serial.println("╔═══════════════════════════════════════════════╗");
//   Serial.println("║   FindSpot DUMMY MODE - No Hardware Needed    ║");
//   Serial.println("╚═══════════════════════════════════════════════╝");
  
//   // Configure watchdog timer
//   Serial.println("\nConfiguring watchdog timer...");
//   esp_task_wdt_config_t wdt_config = {
//     .timeout_ms = WDT_TIMEOUT * 1000,
//     .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
//     .trigger_panic = true
//   };
//   esp_task_wdt_init(&wdt_config);
//   esp_task_wdt_add(NULL);
  
//   // Step 1: Connect to WiFi
//   Serial.println("\nConnecting to WiFi...");
//   wifi.connect();
//   Serial.println("WiFi Connected");
  
//   // Step 2: Synchronize time with NTP server
//   Serial.println("\nSynchronizing time with NTP server...");
//   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//   struct tm timeinfo;
//   int attempts = 0;
//   while (!getLocalTime(&timeinfo) && attempts < 20) {
//     delay(500);
//     Serial.print(".");
//     attempts++;
//   }
//   if (getLocalTime(&timeinfo)) {
//     Serial.println("\nTime synchronized!");
//     Serial.print("   Current time: ");
//     Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
//   } else {
//     Serial.println("\nTime sync failed, continuing anyway...");
//   }
  
//   // Step 3: Register device via HTTP and get MQTT credentials
//   Serial.println("\nRegistering device with backend...");
//   RegistrationResponse regResponse = httpClient.registerDevice(esp32device);
  
//   if (!regResponse.success) {
//     Serial.println("Device registration failed: " + regResponse.error_message);
//     Serial.println("Restarting in 10 seconds...");
//     delay(10000);
//     ESP.restart();
//     return;
//   }
  
//   // Set device ID
//   esp32device.setId(regResponse.device_id);
//   Serial.println("Device registered successfully!");
//   Serial.println("   Device ID: " + String(regResponse.device_id));
  
//   // Step 4: Connect to MQTT broker using credentials from backend
//   Serial.println("\nConnecting to MQTT broker...");
//   mqttClient.setCredentials(
//     regResponse.mqtt_username,
//     regResponse.mqtt_password,
//     regResponse.mqtt_broker,
//     regResponse.mqtt_port,
//     regResponse.sensor_topic,
//     regResponse.device_id
//   );
//   mqttClient.setCallback(mqttCallback);
  
//   // Reset watchdog before MQTT connection attempt
//   esp_task_wdt_reset();
  
//   if (!mqttClient.connect()) {
//     Serial.println("Failed to connect to MQTT broker!");
//     Serial.println("Restarting in 10 seconds...");
//     delay(10000);
//     ESP.restart();
//     return;
//   }
  
//   // Wait for MQTT connection to stabilize
//   unsigned long startWait = millis();
//   while (!mqttClient.isConnected() && (millis() - startWait < 10000)) {
//     mqttClient.loop();
//     delay(100);
//     esp_task_wdt_reset(); // Reset watchdog during connection wait
//   }
  
//   if (!mqttClient.isConnected()) {
//     Serial.println("MQTT connection timeout!");
//     Serial.println("Restarting in 10 seconds...");
//     delay(10000);
//     ESP.restart();
//     return;
//   }
  
//   Serial.println("MQTT Connected");
  
//   // Step 5: Initialize DUMMY sensors
//   Serial.println("\nInitializing DUMMY sensors...");
//   int sensor_id = 0;
  
//   // Create 3 dummy sensors with different cycling patterns
//   sensors.push_back(new DummySensor(esp32device, "ultrasonic_dummy", sensor_id++));
//   sensors.push_back(new DummySensor(esp32device, "ultrasonic_dummy", sensor_id++));
//   sensors.push_back(new DummySensor(esp32device, "ultrasonic_dummy", sensor_id++));

//   for (auto& sensor : sensors) {
//     sensor->begin();
//   }
  
//   Serial.println("Initialized " + String(sensors.size()) + " DUMMY sensors");
//   Serial.println("\nDummy Sensor Patterns:");
//   Serial.println("  Sensor 0: Changes every 3 readings (fast)");
//   Serial.println("  Sensor 1: Changes every 5 readings (medium)");
//   Serial.println("  Sensor 2: Changes in 4/7 pattern (irregular)");
  
//   // Initialize state tracking vector
//   sensorStateVector.resize(sensors.size(), false);
  
//   Serial.println("\n");
//   Serial.println("╔═══════════════════════════════════════════════╗");
//   Serial.println("║    System Ready - Dummy Mode Active          ║");
//   Serial.println("╚═══════════════════════════════════════════════╝\n");
// }

// // ==================== Main Loop ============================ //
// void loop() {
//   // Reset watchdog timer
//   esp_task_wdt_reset();
  
//   // Maintain MQTT connection
//   mqttClient.loop();
  
//   // Check if device is registered
//   if (esp32device.getId() <= 0) {
//     Serial.println("Device not registered, waiting...");
//     delay(1000);
//     return;
//   }
  
//   unsigned long currentMillis = millis();
  
//   // Read sensors periodically
//   if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
//     lastSensorRead = currentMillis;
    
//     Serial.println("\n========== Reading DUMMY Sensors ==========");
//     Serial.print("Free heap: ");
//     Serial.println(ESP.getFreeHeap());
//     yield(); // Allow background tasks
    
//     // Ensure MQTT is connected before reading sensors
//     if (!mqttClient.isConnected()) {
//       Serial.println("MQTT not connected, skipping sensor read");
//       return;
//     }
    
//     // Check each sensor for state changes
//     for (size_t i = 0; i < sensors.size(); i++) {
//       // Safety check: ensure sensor pointer is valid
//       if (!sensors[i]) {
//         Serial.print("ERROR: Null sensor at index ");
//         Serial.println(i);
//         continue;
//       }
      
//       // Safety check: ensure index is within bounds
//       if (i >= sensorStateVector.size()) {
//         Serial.print("ERROR: Index out of bounds ");
//         Serial.println(i);
//         continue;
//       }
      
//       // Add delay between sensors to prevent issues
//       delay(50);
//       yield();
      
//       Serial.print("  Sensor ");
//       Serial.print(i);
//       Serial.print(": ");
      
//       bool currentState = false;
//       // Wrap sensor reading in try-catch equivalent (check for crashes)
//       currentState = sensors[i]->checkState();
      
//       Serial.print(currentState ? "OCCUPIED" : "FREE");
//       Serial.print(" (was: ");
//       Serial.print(sensorStateVector[i] ? "OCCUPIED" : "FREE");
//       Serial.println(")");
      
//       // Only publish if state changed
//       if (currentState != sensorStateVector[i]) {
//         Serial.print("    ↳ State changed! Publishing...");
        
//         yield();
        
//         // Create payload inside a scope to free memory immediately after use
//         String payload = "";
//         if (sensors[i]) {
//           payload = sensors[i]->toJson();
//         }
        
//         if (payload.length() > 0 && mqttClient.isConnected()) {
//           // Add delay to prevent overwhelming MQTT
//           delay(100);
//           yield();
          
//           if (mqttClient.publishSensorData(i, payload)) {
//             sensorStateVector[i] = currentState;
//             Serial.println(" ✓ Published");
//             Serial.println("    ↳ Payload: " + payload);
//           } else {
//             Serial.println(" ✗ Failed");
//           }
//         } else {
//           Serial.println(" ✗ No payload or disconnected");
//         }
        
//         // Force free the payload string
//         payload = String();
//       }
      
//       // Yield after each sensor
//       yield();
//     }
    
//     Serial.println("===========================================\n");
//   }
// }
