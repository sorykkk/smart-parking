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

// Watchdog timeout in seconds (increased to prevent premature resets)
#define WDT_TIMEOUT 30

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
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
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
    Serial.println("\nTime synchronized!");
    Serial.print("   Current time: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
  } else {
    Serial.println("\nTime sync failed, continuing anyway...");
  }
  
  // Reset watchdog before HTTP request
  esp_task_wdt_reset();
  
  // Step 3: Register device via HTTP and get MQTT credentials
  Serial.println("\nRegistering device with backend...");
  RegistrationResponse regResponse = httpClient.registerDevice(esp32device);
  
  // Reset watchdog after HTTP request
  esp_task_wdt_reset();
  
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
  
  // Reset watchdog before MQTT connection attempt
  esp_task_wdt_reset();
  
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
    esp_task_wdt_reset(); // Reset watchdog during connection wait
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
  sensors.push_back(new DistanceSensor(esp32device, "ultrasonic", sensor_id++, 22, 23));
  sensors.push_back(new DistanceSensor(esp32device, "ultrasonic", sensor_id++, 14, 12));
  sensors.push_back(new DistanceSensor(esp32device, "ultrasonic", sensor_id++, 33, 32));

  for (auto& sensor : sensors) {
    sensor->begin();
  }
  
  Serial.println("Initialized " + String(sensors.size()) + " sensors");
  
  // Initialize state tracking vector
  sensorStateVector.resize(sensors.size(), false);
  
  // Publish initial sensor states to register them in the backend
  Serial.println("\nPublishing initial sensor states...");
  delay(1000); // Wait for MQTT to stabilize
  for (size_t i = 0; i < sensors.size(); i++) {
    if (sensors[i] && mqttClient.isConnected()) {
      Serial.print("  Sensor ");
      Serial.print(i);
      Serial.print(": ");
      
      bool initialState = sensors[i]->checkState();
      sensorStateVector[i] = initialState; // Store initial state
      
      String payload = sensors[i]->toJson();
      if (payload.length() > 0) {
        if (mqttClient.publishSensorData(i, payload)) {
          Serial.println(" ✓ Published initial state");
        } else {
          Serial.println(" ✗ Failed to publish");
        }
        delay(200); // Small delay between sensor publishes
      }
    }
  }
  Serial.println("Initial sensor states published");
  
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
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    yield(); // Allow background tasks
    
    // Ensure MQTT is connected before reading sensors
    if (!mqttClient.isConnected()) {
      Serial.println("MQTT not connected, skipping sensor read");
      return;
    }
    
    // Check each sensor for state changes
    for (size_t i = 0; i < sensors.size(); i++) {
      // Safety check: ensure sensor pointer is valid
      if (!sensors[i]) {
        Serial.print("ERROR: Null sensor at index ");
        Serial.println(i);
        continue;
      }
      
      // Safety check: ensure index is within bounds
      if (i >= sensorStateVector.size()) {
        Serial.print("ERROR: Index out of bounds ");
        Serial.println(i);
        continue;
      }
      
      // Add delay between sensors to prevent issues
      delay(50);
      yield();
      
      Serial.print("  Sensor ");
      Serial.print(i);
      Serial.print(": ");
      
      bool currentState = false;
      // Wrap sensor reading in try-catch equivalent (check for crashes)
      currentState = sensors[i]->checkState();
      
      Serial.print(currentState ? "occupied" : "free");
      
      // Only publish if state changed
      if (currentState != sensorStateVector[i]) {
        Serial.print("    State changed! Publishing...");
        
        yield();
        
        // Create payload inside a scope to free memory immediately after use
        String payload = "";
        if (sensors[i]) {
          payload = sensors[i]->toJson();
        }
        
        if (payload.length() > 0 && mqttClient.isConnected()) {
          // Add delay to prevent overwhelming MQTT
          delay(100);
          yield();
          
          if (mqttClient.publishSensorData(i, payload)) {
            sensorStateVector[i] = currentState;
            Serial.println(" ✓ Published");
          } else {
            Serial.println(" ✗ Failed");
          }
        } else {
          Serial.println(" ✗ No payload or disconnected");
        }
        
        // Force free the payload string
        payload = String();
      }
      
      // Yield after each sensor
      yield();
    }
  }
}
