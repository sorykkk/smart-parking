#ifndef DISTANCE_SENSOR_H
#define DISTANCE_SENSOR_H

#include <Arduino.h>
#include "Device.h"
#include "SensorInterface.h"
#include "Config.h"

#define INVALID_DISTANCE -1

namespace FindSpot {

class DistanceSensor : public ISensor {
private:
  int trigPin;
  int echoPin;
  char isoTime[30];
  long lastDistance = INVALID_DISTANCE;
  bool occupied = false;


public:
  DistanceSensor(const Device& device, const String& sensorTech, int sensorIndex, int trig, int echo)
    : trigPin(trig), echoPin(echo) {
      // Initialize base class members
      type = "distance";
      technology = sensorTech;
      index = sensorIndex;
      // Format: ultrasonic_0_esp32_dev_1 (includes device ID)
      name = technology + "_" + String(index) + "_" + device.getName() + "_" + String(device.getId());
      
      // Initialize isoTime with default value
      strcpy(isoTime, "1970-01-01T00:00:00Z");
  }

  void begin() override {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
  }

  /// @brief Calculate the `lastDistance`, and set `occupied` if the parking spot is to consider taken 
  /// @return True if parking spot is occupied (car detected), false otherwise
  bool checkState() override {

    lastDistance = getDistance();
    Serial.print(lastDistance);
    Serial.println(" cm");

    // Occupied when distance is valid AND within the threshold (car is close enough)
    if(lastDistance <= DISTANCE_MAX_CM && lastDistance >= DISTANCE_MIN_CM)
      occupied = true;
    else {
      occupied = false;
    }

    // Update timestamp - simplified to avoid strftime issues
    strcpy(isoTime, "2025-01-01T00:00:00Z");

    return occupied;
  }

  long getDistance() {
    // Disable interrupts temporarily to get a clean reading
    noInterrupts();
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    interrupts();
    
    // Use shorter timeout (30ms) to prevent blocking too long
    long duration = pulseIn(echoPin, HIGH, 30000);
    
    // If pulseIn times out, it returns 0
    if (duration == 0) {
      return INVALID_DISTANCE;
    }
    
    long distance = (duration * 0.034 / 2);

    return (distance < DISTANCE_MIN_CM || distance > DISTANCE_MAX_CM) 
      ? INVALID_DISTANCE 
      : distance;
  }

  String getName() const override { 
    return name; 
  }

  String getType() const override {
    return type;
  }

  int getIndex() const override {
    return index;
  }

  String getTechnology() const override {
    return technology;
  }

  String toJson() const override {
    // Use smaller static JSON document to reduce memory usage
    StaticJsonDocument<256> doc;
    
    doc["name"] = name;
    doc["index"] = index;
    doc["type"] = type;
    doc["technology"] = technology;
    doc["trigger_pin"] = trigPin;
    doc["echo_pin"] = echoPin;
    doc["is_occupied"] = occupied;
    doc["current_distance"] = lastDistance;
    doc["last_updated"] = isoTime;

    String payload;
    payload.reserve(200); // Pre-allocate to avoid fragmentation
    size_t len = serializeJson(doc, payload);
    
    if (len == 0) {
      Serial.println("JSON serialization failed!");
      return "";
    }
    
    return payload;
  }
};
}

#endif
