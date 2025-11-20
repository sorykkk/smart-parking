#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "esp_task_wdt.h"
#include "Config.h"

namespace FindSpot {
class WiFiManager {
public:
  void connect() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    
    int attempts = 0;
    const int maxAttempts = 60; // 30 seconds timeout (60 * 500ms)
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      attempts++;
      
      // Reset watchdog every 5 attempts (2.5 seconds)
      if (attempts % 5 == 0) {
        esp_task_wdt_reset();
      }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nWiFi connection failed!");
      Serial.println("Please check your WiFi credentials and restart.");
    }
  }

  bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
  }
};
}

#endif
