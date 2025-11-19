#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Config.h"

namespace FindSpot {

class MQTTClient {
private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  
  String mqttUsername;
  String mqttPassword;
  String mqttBroker;
  int mqttPort;
  String sensorTopic;
  int deviceId;
  
  unsigned long lastReconnectAttempt;
  static const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds
  
  bool reconnect() {
    if (mqttUsername.isEmpty() || mqttBroker.isEmpty()) {
      Serial.println("MQTT credentials not set");
      return false;
    }
    
    Serial.print("Attempting MQTT connection to " + mqttBroker + "...");
    
    String clientId = String(DEVICE_PREFIX) + String(deviceId);
    
    if (mqttClient.connect(clientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str())) {
      Serial.println(" connected!");
      Serial.println("MQTT Client ID: " + clientId);
      Serial.println("MQTT Username: " + mqttUsername);
      Serial.println("MQTT Keep-Alive: 60s");
      return true;
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds...");
      return false;
    }
  }

public:
  MQTTClient() 
    : mqttClient(wifiClient), 
      lastReconnectAttempt(0),
      deviceId(-1),
      mqttPort(1883) { }

  /**
   * Initialize MQTT client with credentials from HTTP registration
   */
  void setCredentials(const String& username, const String& password, 
                      const String& broker, int port, const String& topic, int devId) {
    mqttUsername = username;
    mqttPassword = password;
    mqttBroker = broker;
    mqttPort = port;
    sensorTopic = topic;
    deviceId = devId;
    
    Serial.println("\nMQTT Configuration:");
    Serial.println("  Broker: " + mqttBroker + ":" + String(mqttPort));
    Serial.println("  Username: " + mqttUsername);
    Serial.println("  Device ID: " + String(deviceId));
    Serial.println("  Sensor Topic: " + sensorTopic);
  }

  /**
   * Connect to MQTT broker
   */
  bool connect() {
    mqttClient.setServer(mqttBroker.c_str(), mqttPort);
    mqttClient.setBufferSize(2048);
    mqttClient.setKeepAlive(60); // Set keep-alive to 60 seconds (default is 15)
    
    Serial.println("\nConnecting to MQTT broker...");
    return reconnect();
  }

  /**
   * Set callback for incoming MQTT messages
   */
  void setCallback(MQTT_CALLBACK_SIGNATURE) {
    mqttClient.setCallback(callback);
  }

  /**
   * Maintain MQTT connection and process messages
   */
  void loop() {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
        lastReconnectAttempt = now;
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
    } else {
      mqttClient.loop();
    }
  }

  /**
   * Publish sensor data to MQTT broker
   * Topic: device/{device_id}/sensors/{sensor_index}
   */
  bool publishSensorData(int sensorIndex, const String& sensorJson) {
    if (!mqttClient.connected()) {
      Serial.println("❌ MQTT not connected, cannot publish");
      Serial.print("   MQTT state: ");
      Serial.println(mqttClient.state());
      return false;
    }
    
    // Validate payload
    if (sensorJson.length() == 0) {
      Serial.println("❌ Empty payload, cannot publish");
      return false;
    }
    
    if (sensorJson.length() > 2048) {
      Serial.println("❌ Payload too large, cannot publish");
      return false;
    }
    
    // Build topic: device/{device_id}/sensors/{sensor_index}
    String topic = "device/" + String(deviceId) + "/sensors/" + String(sensorIndex);
    
    Serial.println("📤 Publishing to MQTT:");
    Serial.println("   Topic: " + topic);
    Serial.println("   Payload: " + sensorJson);
    
    // Convert to c_str early to avoid multiple allocations
    const char* topicCStr = topic.c_str();
    const char* payloadCStr = sensorJson.c_str();
    
    bool result = mqttClient.publish(topicCStr, payloadCStr);
    
    if (!result) {
      Serial.print("❌ Publish failed (rc=");
      Serial.print(mqttClient.state());
      Serial.println(")");
    } else {
      Serial.println("✓ MQTT publish successful");
    }
    
    return result;
  }

  bool isConnected() {
    return mqttClient.connected();
  }

  int getDeviceId() const {
    return deviceId;
  }
};

}

#endif