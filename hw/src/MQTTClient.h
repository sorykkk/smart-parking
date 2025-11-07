#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Device.h"

class MQTTClient {
private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  String deviceId = "";
  String macAddress;
  unsigned long lastReconnectAttempt;
  static const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds
  
  // Registration state
  bool deviceRegistrationComplete;
  bool sensorsRegistrationComplete;
  bool waitingForResponse;
  unsigned long responseTimeout;
  static const unsigned long RESPONSE_TIMEOUT_MS = 10000; // 10 seconds

  bool reconnect() {
    Serial.print("Attempting MQTT connection...");

    // Use MAC address as client ID before registration, device ID after
    String clientId = deviceId.length() > 0 ? deviceId : Device::getMacAddress();
    
    // Build MQTT username: prefix + device_id (or MAC if not registered yet)
    String mqttUser = String(MQTT_USER_PREFIX) + "_" + clientId;
    
    // Attempt to connect with username and password
    if (mqttClient.connect(clientId.c_str(), mqttUser.c_str(), MQTT_PASSWORD)) {
      Serial.println("connected");
      Serial.println("MQTT User: " + mqttUser);
      
      // Subscribe to registration response topic
      String regResponseTopic = String(MQTT_TOPIC_REGISTER_DEVICE) + macAddress + "/response";
      mqttClient.subscribe(regResponseTopic.c_str());
      Serial.println("Subscribed to: " + regResponseTopic);
      
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
    : mqttClient(wifiClient), macAddress(Device::getMacAddress()), lastReconnectAttempt(0), 
      deviceRegistrationComplete(false), sensorsRegistrationComplete(false), waitingForResponse(false), responseTimeout(0) { }

  void begin() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setBufferSize(4096); // Larger buffer for registration payloads
    Serial.println("MQTT Client initialized");
    Serial.println("MAC Address: " + macAddress);
  }

  void loop() {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
        lastReconnectAttempt = now;
        // Attempt to reconnect
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
    
    bool result = mqttClient.publish(topic.c_str(), payload.c_str(), retained);
    if (result) {
      Serial.println("Published to " + topic);
    } else {
      Serial.println("Failed to publish to " + topic);
    }
    return result;
  }

  /**
   * Register device with backend via MQTT
   * Sends MAC address and all sensor/camera information
   */
  bool registerDevice(const Device& device) {
    if (!mqttClient.connected()) {
      Serial.println("MQTT not connected, cannot register");
      return false;
    }

    Serial.println("\n=== Device Registration via MQTT ===");
 
    Serial.println("Device registration payload:");
    String payload = device.toJson();
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
    DynamicJsonDocument doc(8192);
    for (const auto& sensor : sensors) {
      String type = sensor->getType();
      JsonArray arr = doc[type];
      if(arr.isNull()) {
        arr = doc.createNestedArray(type);
      }
      
      JsonObject sensorObj = arr.createNestedObject();
      // Parse the sensor JSON and add device_id
      StaticJsonDocument<512> sensorDoc;
      deserializeJson(sensorDoc, sensor->toJson());
      sensorObj["device_id"] = deviceId;
      for (JsonPair kv : sensorDoc.as<JsonObject>()) {
        sensorObj[kv.key()] = kv.value();
      }
    }

    String output;
    serializeJson(doc, output);

    Serial.println("Sensors registration payload:");
    Serial.println(output);
    
    // Publish to registration request topic
    String topic = String(MQTT_TOPIC_REGISTER_SENSORS) + "request";
    bool result = publish(topic, output);
    
    if (result) {
      waitingForResponse = true;
      responseTimeout = millis();
      Serial.println("Sensors registration request sent, waiting for response...");
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
        if(doc.containsKey("device_id")) {
          deviceId = doc["device_id"].as<String>();
          Serial.println("Device ID received: " + deviceId);
        }
      } else {
        // This is sensors registration response  
        sensorsRegistrationComplete = true;
        int distance_sensors_reg = doc["sensors_registered"].as<int>();
        int camera_sensors_reg = doc["cameras_registered"].as<int>(); 
        Serial.println("Sensors registration confirmed.\nRegistered: " + (distance_sensors_reg + camera_sensors_reg) + " total sensors.\n");
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

    String topic = String(MQTT_TOPIC_SENSORS) + deviceId + "/data/" + doc["index"].as<String>();
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

  String getDeviceId() {
    return deviceId;
  }

  String getMac() {
    return macAddress;
  }


  PubSubClient& getClient() {
    return mqttClient;
  }
};

#endif // MQTT_CLIENT_H
