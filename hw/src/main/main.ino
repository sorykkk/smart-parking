#include <Arduino.h>
#include <vector>
#include "../Config.h"
#include "../WiFiManager.h"
#include "../DistanceSensor.h"
#include "../CameraDevice.h"
#include "../HttpClient.h"

WiFiManager wifi;
HttpClient client;

std::vector<DistanceSensor> sensors;
std::vector<CameraDevice> cameras;

void setup() {
  // TODO: make the names for sensor to be configurable 
  // extracted from database, and only thing you should do
  // is to fetch data last id from database and increment it
  // and form the new name
  Serial.begin(115200);
  wifi.connect();

  String dist_sensor_name = "dist-sensor-";
  int fetched_dist_inc_id = 0;

  String cam_dev_name = "esp-cam-";
  int fetched_cam_inc_id = 0;

  sensors.emplace_back(dist_sensor_name + String(++fetched_dist_inc_id), 12, 13);
  sensors.emplace_back(dist_sensor_name + String(++fetched_dist_inc_id), 14, 15);
  for (auto& s : sensors) s.begin();

  cameras.emplace_back(cam_dev_name + String(++fetched_cam_inc_id), FRAMESIZE_QVGA, 12);
  for (auto& c : cameras) c.begin();

  // push new registered devices to DB
}

void loop() {
  // Read all sensors
  for (auto& s : sensors)
    s.readDistance();

  // Capture camera image (without base64 for now)
  for (auto& c : cameras)
    c.capture(false);

  // Build full JSON payload
  String payload = "{";

  payload += "\"sensors\":[";
  for (size_t i = 0; i < sensors.size(); ++i) {
    payload += sensors[i].toJSON();
    if (i < sensors.size() - 1) payload += ",";
  }
  payload += "],";

  payload += "\"cameras\":[";
  for (size_t i = 0; i < cameras.size(); ++i) {
    payload += cameras[i].toJSON(false);
    if (i < cameras.size() - 1) payload += ",";
  }
  payload += "]}";

  Serial.println("Payload:");
  Serial.println(payload);

  client.postJSON(payload);
  delay(3000);
}
