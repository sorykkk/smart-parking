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
      // Device is already registered at this point, so device.getId() is valid
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
    bool previousState = occupied;
    lastDistance = getDistance();
    // Occupied when distance is valid AND within the threshold (car is close enough)
    occupied = (lastDistance != INVALID_DISTANCE && lastDistance <= DISTANCE_MAX_CM);

    // Log state change
    if (occupied != previousState) {
      Serial.print("Sensor ");
      Serial.print(name);
      Serial.print(" (index ");
      Serial.print(index);
      Serial.print("): ");
      Serial.print(previousState ? "occupied" : "free");
      Serial.print(" -> ");
      Serial.print(occupied ? "occupied" : "free");
      Serial.print(" (distance: ");
      Serial.print(lastDistance);
      Serial.println("cm)");
    }

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
    } else {
      strcpy(isoTime, "1970-01-01T00:00:00Z"); // fallback
    }

    return occupied;
  }

  long getDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Use shorter timeout (25ms) to prevent blocking too long
    long duration = pulseIn(echoPin, HIGH, 25000);
    
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
    // Build registration JSON payload - use smaller doc, heap allocation
    DynamicJsonDocument* doc = new DynamicJsonDocument(512);
    if (!doc) {
      return "{}";  // Return empty object on allocation failure
    }
    
    (*doc)["name"] = name;
    (*doc)["index"] = index;
    (*doc)["type"] = type;
    (*doc)["technology"] = technology;
    (*doc)["trigger_pin"] = trigPin;
    (*doc)["echo_pin"] = echoPin;
    (*doc)["is_occupied"] = occupied;
    (*doc)["current_distance"] = lastDistance;
    (*doc)["last_updated"] = isoTime;

    String payload;
    serializeJson(*doc, payload);
    delete doc;  // Free heap memory
    return payload;
  }

  void toJsonObject(JsonObject& obj) const override {
    obj["name"] = name;
    obj["index"] = index;
    obj["type"] = type;
    obj["technology"] = technology;
    obj["trigger_pin"] = trigPin;
    obj["echo_pin"] = echoPin;
    obj["is_occupied"] = occupied;
    obj["current_distance"] = lastDistance;
    obj["last_updated"] = isoTime;
  }
};
}

#endif
