#ifndef DISTANCE_SENSOR_H
#define DISTANCE_SENSOR_H

#include <Arduino.h>

class DistanceSensor {
private:
  int trigPin;
  int echoPin;
  String name;
  long lastDistance = -1;

public:
  DistanceSensor(const String& sensorName, int trig, int echo)
    : name(sensorName), trigPin(trig), echoPin(echo) {}

  void begin() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
  }

  long readDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 30000);
    lastDistance = duration * 0.034 / 2;
    return lastDistance;
  }

  String getName() const { return name; }

  String toJSON() const {
    // Example: {"name":"FrontSensor","distance":123}
    String json = "{\"name\":\"" + name + "\",\"distance\":" + String(lastDistance) + "}";
    return json;
  }
};

#endif
