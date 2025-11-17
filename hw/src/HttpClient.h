#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Device.h"
#include "Config.h"

namespace FindSpot {

struct RegistrationResponse {
  bool success;
  int device_id;
  String mqtt_username;
  String mqtt_password;
  String mqtt_broker;
  int mqtt_port;
  String sensor_topic;
  String error_message;
};

class HttpClient {
private:
  HTTPClient http;
  
public:
  /**
   * Register device with backend via HTTP
   * Returns device_id and MQTT credentials
   */
  RegistrationResponse registerDevice(const Device& device) {
    RegistrationResponse response;
    response.success = false;
    response.device_id = -1;
    
    if (WiFi.status() != WL_CONNECTED) {
      response.error_message = "WiFi not connected";
      return response;
    }
    
    Serial.println("Registering device with backend via HTTP...");
    
    // Build registration URL
    String url = String("http://") + BACKEND_HOST + ":" + String(BACKEND_PORT) + "/" + String(BACKEND_REGISTER_URL);
    
    // Create registration JSON payload
    StaticJsonDocument<512> doc;
    doc["mac_address"] = device.getMacAddress();
    doc["name"] = device.getName();
    doc["location"] = device.getLocation();
    doc["latitude"] = device.getLatitude();
    doc["longitude"] = device.getLongitude();
    
    String json;
    serializeJson(doc, json);
    
    Serial.println("Sending to: " + url);
    Serial.println("Payload: " + json);
    
    // Send HTTP POST request
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(json);
    String responseBody = http.getString();
    http.end();
    
    Serial.println("HTTP Response Code: " + String(httpCode));
    Serial.println("Response: " + responseBody);
    
    // Parse response
    if (httpCode == 200 || httpCode == 201) {
      StaticJsonDocument<1024> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, responseBody);
      
      if (!error) {
        response.success = true;
        response.device_id = responseDoc["device_id"];
        response.mqtt_username = responseDoc["mqtt_username"].as<String>();
        response.mqtt_password = responseDoc["mqtt_password"].as<String>();
        response.mqtt_broker = responseDoc["mqtt_broker"].as<String>();
        response.mqtt_port = responseDoc["mqtt_port"];
        response.sensor_topic = responseDoc["sensor_topic"].as<String>();
        
        Serial.println("Registration successful!");
        Serial.println("Device ID: " + String(response.device_id));
        Serial.println("MQTT Username: " + response.mqtt_username);
        Serial.println("Sensor Topic: " + response.sensor_topic);
      } else {
        response.error_message = "Failed to parse response JSON";
        Serial.println(response.error_message);
      }
    } else {
      response.error_message = "HTTP error " + String(httpCode) + ": " + responseBody;
      Serial.println("Registration failed: " + response.error_message);
    }
    
    return response;
  }
};

}

#endif
