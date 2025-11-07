#ifndef DISTANCE_SENSOR_H
#define DISTANCE_SENSOR_H

#include <Arduino.h>
#include "Device.h"
#include "SensorInterface.h"

#define INVALID_DISTANCE -1

class DistanceSensor : public ISensor {
private:
  int trigPin;
  int echoPin;
  char isoTime[30];

  long lastDistance = INVALID_DISTANCE;
  bool occupied = false;


public:
  DistanceSensor(const Device& device, const String& sensorTech, int sensorIndex, int trig, int echo)
    : type("distance"), technology(sensorTech), index(sensorIndex), trigPin(trig), echoPin(echo) {
      //ultrasonic_1_esp32_1
      name = technology + "_" + String(index) + "_" + device.getName() + "_" + device.getId();
  }

  void begin() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
  }

  /// @brief Calculate the `lastDistance`, and set `occupied` if the parking stop is to consider taken 
  /// @return Distance in cm
  bool checkState() override {
    lastDistance = getDistance();
    occupied = (lastDistance == INVALID_DISTANCE)? false : true;

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
    long duration = pulseIn(echoPin, HIGH, 30000);
    long distance = (duration * 0.034 / 2);

    return (distance < DISTANCE_MIN_CM && distance > DISTANCE_MAX_CM) 
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

  String toJson() const override {
    // Build registration JSON payload
    DynamicJsonDocument doc(4096);
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
    serializeJson(doc, payload);
    return payload;
  }
};

#endif
