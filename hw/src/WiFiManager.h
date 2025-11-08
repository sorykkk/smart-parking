#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "Config.h"

namespace FindSpot {
class WiFiManager {
public:
  void connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.println(WiFi.localIP());
  }

  bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
  }
};
}

#endif
