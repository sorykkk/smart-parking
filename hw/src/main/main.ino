#include <Arduino.h>
#include <vector>
#include "../Config.h"
#include "../WiFiManager.h"
#include "../DistanceSensor.h"
#include "../CameraDevice.h"
#include "../MQTTClient.h"
#include "time.h"

using namespace FindSpot;

WiFiManager wifi;
MQTTClient mqttClient;  // No device ID needed, will be retrieved
Device *esp32device = nullptr; // Global pointer to device

std::vector<ISensor*> sensors;
std::vector<bool> stateVector; // Track sensor states

unsigned long lastSensorRead = 0;

// NTP server and timezone
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;      // for GMT+2
const int   daylightOffset_sec = 3600; // use 0 if none

/**
 * MQTT callback for incoming messages
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("MQTT Message on topic: " + String(topic));
  Serial.println("Payload: " + message);
  
  // Check if this is a registration response
  String macAddress = mqttClient.getMac();
  String regResponseTopic = String(MQTT_TOPIC_REGISTER_DEVICE) + macAddress + "/response";
  String sensorsRegResponseTopic = String(MQTT_TOPIC_REGISTER_SENSORS) + macAddress + "/response";
  
  if (String(topic) == regResponseTopic || String(topic) == sensorsRegResponseTopic) {
    if(!mqttClient.handleRegistrationResponse(message)) {
      Serial.println("Could not finish registration");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== FindSpot Smart Parking System ===");
  
  // Connect to WiFi
  Serial.println("\n--- Connecting to WiFi ---");
  wifi.connect();
  
  Serial.println("\n--- Initializing ESP32 devices ---");
  esp32device = new Device(MQTT_DEVICE_PREFIX, DEVICE_LOCATION, DEVICE_LATITUDE, DEVICE_LONGITUDE);
  
  // Set device in MQTT client BEFORE begin() so MAC address is available
  mqttClient.setDevice(*esp32device);
  
  // Initialize MQTT client
  Serial.println("\n--- Initializing MQTT Client ---");
  mqttClient.begin();
  mqttClient.setCallback(mqttCallback);
  
  // Wait for MQTT connection
  Serial.println("Waiting for MQTT connection...");
  unsigned long startWait = millis();
  while (!mqttClient.isConnected() && (millis() - startWait < 30000)) {
    mqttClient.loop();
    delay(500);
  }
  
  if (!mqttClient.isConnected()) {
    Serial.println("Failed to connect to MQTT broker!");
    Serial.println("Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
  }
  
  Serial.println("MQTT Connected");

  Serial.println("\n--- Synchronizing time ---");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTime synchronized!");
  
  // First: Register device and wait for device ID
  mqttClient.registerDevice(*esp32device);
  
  // Wait for device registration response
  Serial.println("Waiting for device registration response...");
  unsigned long deviceRegStart = millis();
  while (!mqttClient.isDeviceRegistered() && (millis() - deviceRegStart < 15000)) {
    mqttClient.loop();
    delay(100);
  }
  
  if (!mqttClient.isDeviceRegistered()) {
    Serial.println("Warning: Device registration timed out");
    Serial.println("Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
  }

  // Update device with received ID
  int receivedDeviceId = mqttClient.getDeviceId();
  esp32device->setId(receivedDeviceId);
  Serial.println("Device registered with ID: " + String(receivedDeviceId));
  
  // CRITICAL: Give the MQTT client time to stabilize after credential switch
  Serial.println("Allowing MQTT client to stabilize after credential switch...");
  delay(2000);
  for (int i = 0; i < 10; i++) {
    mqttClient.loop();
    delay(100);
  }
  Serial.println("MQTT client stabilized");
  
  // Initialize sensors AFTER getting device ID
  Serial.println("\n--- Initializing Sensors ---");
  int sensor_id = 0;
  
  // Setup ultrasonic sensors (each represents a parking spot)
  sensors.push_back(new DistanceSensor(*esp32device, "ultrasonic", sensor_id++, 13, 12));
  sensors.push_back(new DistanceSensor(*esp32device, "ultrasonic", sensor_id++, 14, 15));
  sensors.push_back(new DistanceSensor(*esp32device, "ultrasonic", sensor_id++, 16, 0));

  for (auto& s : sensors) {
    s->begin();
  }
  Serial.println("Initialized " + String(sensors.size()) + " sensors");
  
  // Initialize state tracking vector
  stateVector.resize(sensors.size(), false);

  // Second: Register sensors with device ID
  mqttClient.registerSensors(sensors, *esp32device);
  
  // Wait for sensors registration response
  Serial.println("Waiting for sensors registration response...");
  unsigned long sensorsRegStart = millis();
  while (!mqttClient.isSensorsRegistered() && (millis() - sensorsRegStart < 15000)) {
    mqttClient.loop();
    delay(100);
  }
  
  if (!mqttClient.isSensorsRegistered()) {
    Serial.println("Warning: Sensors registration timed out");
    Serial.println("Will continue but sensor data may not be processed correctly");
  } else {
    Serial.println("Sensors registered successfully");
  }
  
  Serial.println("\n=== Setup Complete ===");
  Serial.println("MAC Address: " + mqttClient.getMac());
  Serial.println("ESP32 ID: " + mqttClient.getDeviceId());
  Serial.println("Sensors: " + String(sensors.size()));
  Serial.println("======================\n");
}

void loop() {
  // Maintain MQTT connection
  mqttClient.loop();
  
  // Only publish sensor data if both device and sensors are registered
  if (!mqttClient.isDeviceRegistered() || !mqttClient.isSensorsRegistered()) {
    delay(1000);
    return;
  }
  
  unsigned long currentMillis = millis();
  
  // Read sensors periodically
  if (currentMillis - lastSensorRead >= DISTANCE_SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    
    // Read all sensors and check for state changes
    for (size_t i = 0; i < sensors.size(); i++) {
      bool currentState = sensors[i]->checkState();
      if(currentState != stateVector[i]) {
        Serial.println("Sensor state changed - Publishing sensor data:");
        String payload = sensors[i]->toJson();
        Serial.println(payload);
        
        if(mqttClient.publishSensorData(payload)) {
          stateVector[i] = currentState;
          Serial.println("Sensor data published successfully");
        } else {
          Serial.println("Failed to publish sensor data");
        }
      }
    }
  }
}
