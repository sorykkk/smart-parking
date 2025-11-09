#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"  // For SHA256
#include "Config.h"
#include "Device.h"

namespace FindSpot {
class MQTTClient {
private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  // device info
  int deviceId = -1;
  String macAddress;  // Store MAC address

  unsigned long lastReconnectAttempt;
  static const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds
  
  // Registration state
  bool deviceRegistrationComplete;
  bool sensorsRegistrationComplete;
  bool waitingForResponse;
  unsigned long responseTimeout;
  static const unsigned long RESPONSE_TIMEOUT_MS = 10000; // 10 seconds

  // Generate deterministic MQTT credentials matching backend logic
  String generateMQTTUsername() {
    String cleanMac = macAddress;
    cleanMac.replace(":", "");
    cleanMac.toLowerCase();
    return String(MQTT_DEVICE_PREFIX) + "_" + cleanMac;  // Use configurable prefix
  }

  String generateMQTTPassword() {
    // Generate password matching backend SHA256 logic
    // Backend: password_data = f"{clean_mac}{mqtt_salt}"
    // Backend: hashlib.sha256(password_data.encode()).hexdigest()[:32]
    
    String cleanMac = macAddress;
    cleanMac.replace(":", "");
    cleanMac.toLowerCase();
    
    String combined = cleanMac + String(MQTT_PASS);
    
    // Use mbedTLS SHA256
    byte hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)combined.c_str(), combined.length());
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);
    
    // Convert to hex string (first 32 characters to match backend)
    String password = "";
    for (int i = 0; i < 16; i++) {  // 16 bytes = 32 hex chars
      char hex[3];
      sprintf(hex, "%02x", hash[i]);
      password += String(hex);
    }
    
    return password;
  }

  bool reconnect() {
    if (macAddress.isEmpty()) {
      Serial.println("Cannot reconnect: Device MAC address not set");
      return false;
    }
    
    Serial.print("Attempting MQTT connection...");

    String clientId = macAddress;
    String mqttUser, mqttPassword;
    
    if (!deviceRegistrationComplete) {
      // Use backend credentials for initial registration
      mqttUser = BACKEND_MQTT_USER;
      mqttPassword = BACKEND_MQTT_PASS;
      clientId = String(MQTT_DEVICE_PREFIX) + "_reg_" + macAddress;
      Serial.println("Using backend credentials for registration");
    } else {
      // Use device credentials for normal operation
      mqttUser = generateMQTTUsername();
      mqttPassword = generateMQTTPassword();
      Serial.println("Using device credentials");
    }
    
    // Attempt to connect with appropriate credentials
    if (mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str())) {
      Serial.println("connected");
      Serial.println("MQTT User: " + mqttUser);
      
      // Subscribe to registration response topics
      String regResponseTopic = String(MQTT_TOPIC_REGISTER_DEVICE) + macAddress + "/response";
      mqttClient.subscribe(regResponseTopic.c_str());
      Serial.println("Subscribed to: " + regResponseTopic);
      
      String sensorsRegResponseTopic = String(MQTT_TOPIC_REGISTER_SENSORS) + macAddress + "/response";
      mqttClient.subscribe(sensorsRegResponseTopic.c_str());
      Serial.println("Subscribed to: " + sensorsRegResponseTopic);
      
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      return false;
    }
  }

public:
  MQTTClient() 
    : mqttClient(wifiClient), lastReconnectAttempt(0), 
      deviceRegistrationComplete(false), sensorsRegistrationComplete(false), waitingForResponse(false), responseTimeout(0) { }

  void setDevice(Device& device) {
    macAddress = device.getMacAddress();  // Store MAC address
  }

  String getMac() {
    return macAddress;
  }

  void begin() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setBufferSize(4096); // Larger buffer for registration payloads
    Serial.println("MQTT Client initialized");
    Serial.println("MAC Address: " + macAddress);
  }

  void setCallback(MQTT_CALLBACK_SIGNATURE) {
    mqttClient.setCallback(callback);
  }

  void loop() {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
        lastReconnectAttempt = now;
        // Attempt to reconnect using stored device
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
    } else {
      mqttClient.loop();
    }
    
    // Check for response timeout
    if (waitingForResponse && (millis() - responseTimeout > RESPONSE_TIMEOUT_MS)) {
      Serial.println("Registration response timeout!");
      waitingForResponse = false;
    }
  }

  bool publish(const String& topic, const String& payload, bool retained = false) {
    if (!mqttClient.connected()) {
      Serial.println("MQTT not connected, cannot publish");
      return false;
    }
    
    // Give system time to handle other tasks
    yield();
    delay(10);
    
    bool result = mqttClient.publish(topic.c_str(), payload.c_str(), retained);
    
    yield();
    
    if (result) {
      Serial.print("Published to ");
      Serial.println(topic);
    } else {
      Serial.print("Failed to publish to ");
      Serial.println(topic);
    }
    return result;
  }

  /**
   * Register device with backend via MQTT
   * Sends MAC address, credentials, and all device information
   */
  bool registerDevice(const Device& device) {
    Serial.println("\n=== Device Registration via MQTT ===");
    
    // For initial registration, we need to connect first
    if (!mqttClient.connected()) {
      Serial.println("MQTT not connected, attempting to connect for registration...");
      if (!reconnect()) {
        Serial.println("Failed to connect to MQTT for registration");
        return false;
      }
    }
 
    // Get device payload
    String devicePayload = device.toJson();
    
    // Parse it to add MQTT credentials
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, devicePayload);
    
    // Add MQTT credentials that ESP32 is using
    doc["mqtt_username"] = generateMQTTUsername();
    doc["mqtt_password"] = generateMQTTPassword();
    
    String payload;
    serializeJson(doc, payload);
    
    Serial.println("Device registration payload:");
    Serial.println(payload);
    
    // Publish to registration request topic
    String topic = String(MQTT_TOPIC_REGISTER_DEVICE) + "request";
    bool result = publish(topic, payload);
    
    if (result) {
      waitingForResponse = true;
      responseTimeout = millis();
      Serial.println("Device registration request sent, waiting for response...");
    } else {
      Serial.println("Failed to send device registration request");
    }
    
    return result;
  }

  bool registerSensors(const std::vector<ISensor*>& sensors, const Device& device) {
    if (!mqttClient.connected()) {
      Serial.println("MQTT not connected, cannot register");
      return false;
    }

    Serial.println("\n=== Sensors Registration via MQTT ===");
    Serial.print("Free heap before allocation: ");
    Serial.println(ESP.getFreeHeap());
    
    // Allocate JSON document
    DynamicJsonDocument* doc = new DynamicJsonDocument(2048);
    if (!doc) {
      Serial.println("Failed to allocate memory for sensor registration");
      return false;
    }
    
    Serial.print("Free heap after doc allocation: ");
    Serial.println(ESP.getFreeHeap());
    
    // Populate the document
    for (const auto& sensor : sensors) {
      if (!sensor) continue;
      
      String typeStr = sensor->getType();
      JsonArray arr = (*doc)[typeStr];
      if(arr.isNull()) {
        arr = doc->createNestedArray(typeStr);
      }
      
      JsonObject sensorObj = arr.createNestedObject();
      sensorObj["device_id"] = deviceId;
      sensorObj["name"] = sensor->getName();
      sensorObj["index"] = sensor->getIndex();
      sensorObj["type"] = typeStr;
      sensorObj["technology"] = sensor->getTechnology();
    }

    // Serialize directly to a String object. This is the simplest and safest way.
    String payload;
    payload.reserve(measureJson(*doc) + 1); // Pre-allocate memory
    size_t len = serializeJson(*doc, payload);

    // The document is no longer needed
    delete doc;
    doc = nullptr;

    if (len == 0) {
      Serial.println("Failed to serialize JSON");
      return false;
    }

    Serial.println("--- Payload Ready ---");
    Serial.print("JSON size: ");
    Serial.println(len);
    Serial.println(payload);

    // Use the exact same working pattern as registerDevice
    String topic = String(MQTT_TOPIC_REGISTER_SENSORS) + "request";
    
    // Use the simple, reliable publish helper method
    bool result = publish(topic, payload);
    
    if (result) {
      Serial.println("Sensors registration request sent, waiting for response...");
      waitingForResponse = true;
      responseTimeout = millis();
    } else {
      Serial.println("Failed to send sensors registration request");
    }
    
    return result;
  }  


  bool handleRegistrationResponse(const String& payload) {
    Serial.println("Received registration response:");
    Serial.println(payload);
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.println("Failed to parse registration response");
      waitingForResponse = false;
      return false;
    }
    
    if(!doc.containsKey("status")) {
      Serial.println("Missing 'status' key in response. Invalid response");
      return false;
    }

    String status = doc["status"].as<String>();
    Serial.println("Status: " + status);
  
    if (status == "registered") {
      if(deviceRegistrationComplete == false) {
        // This is device registration response
        deviceRegistrationComplete = true;
        if(doc.containsKey("id")) {  // Backend sends 'id', not 'device_id'
          deviceId = doc["id"].as<int>();
          Serial.println("Device ID received: " + String(deviceId));
        }
        
        Serial.println("Device registration complete!");
        Serial.println("Switching to device credentials...");
        
        // Disconnect and reconnect with device credentials
        mqttClient.disconnect();
        delay(1000);
        
        // Reconnect will now use device credentials since deviceRegistrationComplete = true
        if (reconnect()) {
          Serial.println("Successfully switched to device credentials");
        } else {
          Serial.println("Failed to reconnect with device credentials");
        }
        
      } else {
        // This is sensors registration response  
        sensorsRegistrationComplete = true;
        int distance_sensors_reg = doc["sensors_registered"].as<int>();
        int camera_sensors_reg = doc["cameras_registered"].as<int>(); 
        String total = String(distance_sensors_reg + camera_sensors_reg);
        Serial.println("Sensors registration confirmed.\nRegistered: " + total + " total sensors.\n");
      }
      waitingForResponse = false;
      return true;
    }
    else if(status == "error") {
      String message = doc["message"].as<String>();
      Serial.println("[Backend error]: " + message);
    }

    return false;
  }

  bool publishSensorData(const String& sensorJson) {
    if (!sensorsRegistrationComplete) {
      Serial.println("Sensors not registered yet, skipping sensor data");
      return false;
    }

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, sensorJson);

    if (error) {
      Serial.println("Failed to parse sensor data");
      return false;
    }

    String topic = String(MQTT_TOPIC_SENSORS) + String(deviceId) + "/data/" + doc["index"].as<String>();
    return publish(topic, sensorJson);
  }

  bool isConnected() {
    return mqttClient.connected();
  }

  bool isDeviceRegistered() {
    return deviceRegistrationComplete;
  }

  bool isSensorsRegistered() {
    return sensorsRegistrationComplete;
  }

  bool isWaitingForResponse() {
    return waitingForResponse;
  }

  int getDeviceId() {
    return deviceId;
  }
};
}

#endif