#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include "Config.h"

class HttpClient {
public:
  bool postJSON(const String& json) {
    if (WiFi.status() != WL_CONNECTED) return false;
    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json);
    http.end();
    return code > 0;
  }
};

#endif
