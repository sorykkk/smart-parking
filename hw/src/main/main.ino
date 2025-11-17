#include <Arduino.h>
#include <vector>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_task_wdt.h"
#include "../Config.h"
#include "../WiFiManager.h"
#include "../DistanceSensor.h"
#include "../CameraDevice.h"
#include "../MQTTClient.h"
#include "../HttpClient.h"
#include "time.h"

using namespace FindSpot;

// Watchdog timeout in seconds
#define WDT_TIMEOUT 10

// ==================== Global Objects ============================ //
WiFiManager wifi;
HttpClient httpClient;
MQTTClient mqttClient;
Device esp32device(DEVICE_PREFIX, DEVICE_LOCATION, DEVICE_LATITUDE, DEVICE_LONGITUDE);

std::vector<ISensor*> sensors;
std::vector<bool> sensorStateVector;

unsigned long lastSensorRead = 0;

// NTP server and timezone settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;      // GMT+2
const int   daylightOffset_sec = 3600; // Daylight saving

// ==================== MQTT Callback ============================ //
/**
 * MQTT callback for incoming messages (optional - for future features)
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("📨 MQTT Message on topic: " + String(topic));
  Serial.println("📦 Payload: " + message);
}

// ==================== Setup ============================ //
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════════════╗");
  Serial.println("║   FindSpot Smart Parking System - ESP32       ║");
  Serial.println("╚═══════════════════════════════════════════════╝");
  
  // Configure watchdog timer
  Serial.println("\nConfiguring watchdog timer...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  // Step 1: Connect to WiFi
  Serial.println("\nConnecting to WiFi...");
  wifi.connect();
  Serial.println("WiFi Connected");
  
  // Step 2: Synchronize time with NTP server
  Serial.println("\nSynchronizing time with NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (getLocalTime(&timeinfo)) {
    Serial.println("\nime synchronized!");
    Serial.print("   Current time: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
  } else {
    Serial.println("\nTime sync failed, continuing anyway...");
  }
  
  // Step 3: Register device via HTTP and get MQTT credentials
  Serial.println("\nRegistering device with backend...");
  RegistrationResponse regResponse = httpClient.registerDevice(esp32device);
  
  if (!regResponse.success) {
    Serial.println("Device registration failed: " + regResponse.error_message);
    Serial.println("Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
    return;
  }
  
  // Set device ID
  esp32device.setId(regResponse.device_id);
  Serial.println("Device registered successfully!");
  Serial.println("   Device ID: " + String(regResponse.device_id));
  
  // Step 4: Connect to MQTT broker using credentials from backend
  Serial.println("\nConnecting to MQTT broker...");
  mqttClient.setCredentials(
    regResponse.mqtt_username,
    regResponse.mqtt_password,
    regResponse.mqtt_broker,
    regResponse.mqtt_port,
    regResponse.sensor_topic,
    regResponse.device_id
  );
  mqttClient.setCallback(mqttCallback);
  
  if (!mqttClient.connect()) {
    Serial.println("Failed to connect to MQTT broker!");
    Serial.println("Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
    return;
  }
  
  // Wait for MQTT connection to stabilize
  unsigned long startWait = millis();
  while (!mqttClient.isConnected() && (millis() - startWait < 10000)) {
    mqttClient.loop();
    delay(100);
  }
  
  if (!mqttClient.isConnected()) {
    Serial.println("MQTT connection timeout!");
    Serial.println("Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
    return;
  }
  
  Serial.println("MQTT Connected");
  
  // Step 5: Initialize sensors
  Serial.println("\nInitializing sensors...");
  int sensor_id = 0;
  
  // Setup ultrasonic sensors (each represents a parking spot)
  // Format: DistanceSensor(device, technology, index, trigger_pin, echo_pin)
  sensors.push_back(new DistanceSensor(esp32device, "ultrasonic", sensor_id++, 13, 12));
  sensors.push_back(new DistanceSensor(esp32device, "ultrasonic", sensor_id++, 14, 15));
  sensors.push_back(new DistanceSensor(esp32device, "ultrasonic", sensor_id++, 16, 0));

  for (auto& sensor : sensors) {
    sensor->begin();
  }
  
  Serial.println("Initialized " + String(sensors.size()) + " sensors");
  
  // Initialize state tracking vector
  sensorStateVector.resize(sensors.size(), false);
  
  Serial.println("\n");
  Serial.println("╔═══════════════════════════════════════════════╗");
  Serial.println("║          System Ready - Starting Loop         ║");
  Serial.println("╚═══════════════════════════════════════════════╝\n");
}

// ==================== Main Loop ============================ //
void loop() {
  // Reset watchdog timer
  esp_task_wdt_reset();
  
  // Maintain MQTT connection
  mqttClient.loop();
  
  // Check if device is registered
  if (esp32device.getId() <= 0) {
    Serial.println("Device not registered, waiting...");
    delay(1000);
    return;
  }
  
  unsigned long currentMillis = millis();
  
  // Read sensors periodically
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    
    Serial.println("\nReading sensors...");
    
    // Check each sensor for state changes
    for (size_t i = 0; i < sensors.size(); i++) {
      // Safety check: ensure sensor pointer is valid
      if (!sensors[i]) {
        Serial.println("Warning: Null sensor at index " + String(i));
        continue;
      }
      
      // Safety check: ensure index is within bounds
      if (i >= sensorStateVector.size()) {
        Serial.println("Warning: Index out of bounds " + String(i));
        continue;
      }
      
      bool currentState = sensors[i]->checkState();
      
      // Only publish if state changed
      if (currentState != sensorStateVector[i]) {
        Serial.println("Sensor " + String(i) + " state changed: " + 
                      String(sensorStateVector[i] ? "occupied" : "free") + " -> " +
                      String(currentState ? "occupied" : "free"));
        
        String payload = sensors[i]->toJson();
        Serial.println("Publishing sensor data for sensor " + String(i));
        
        // Add small delay to prevent overwhelming MQTT
        delay(10);
        
        if (mqttClient.publishSensorData(i, payload)) {
          sensorStateVector[i] = currentState;
          Serial.println("Sensor " + String(i) + " data published successfully");
        } else {
          Serial.println("Failed to publish sensor " + String(i) + " data");
        }
      }
    }
  }
}
