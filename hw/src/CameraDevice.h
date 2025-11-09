#ifndef CAMERA_DEVICE_H
#define CAMERA_DEVICE_H

#include "esp_camera.h"
#include <Arduino.h>
#include <base64.h>  // Built-in Arduino Base64 helper
#include "SensorInterface.h"
#include <ArduinoJson.h>

namespace FindSpot {
class CameraDevice : ISensor {
private:
  framesize_t frameSize;
  int jpegQuality;
  size_t lastImageSize = 0;
  String lastImageBase64;  // optional preview
  char isoTime[30];

public:
  CameraDevice(const Device& device, const String& sensorTech, int sensorIndex, framesize_t size = FRAMESIZE_QVGA, int quality = 12)
      : frameSize(size), jpegQuality(quality) {
        // Initialize base class members
        type = "camera";
        technology = sensorTech;
        index = sensorIndex;
        //camera_1_esp32_1
        name = technology + "_" + String(index) + "_" + device.getName() + "_" + device.getId();
  }

  void begin() override {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = 5;
    config.pin_d1       = 18;
    config.pin_d2       = 19;
    config.pin_d3       = 21;
    config.pin_d4       = 36;
    config.pin_d5       = 39;
    config.pin_d6       = 34;
    config.pin_d7       = 35;
    config.pin_xclk     = 0;
    config.pin_pclk     = 22;
    config.pin_vsync    = 25;
    config.pin_href     = 23;
    config.pin_sscb_sda = 26;
    config.pin_sscb_scl = 27;
    config.pin_pwdn     = 32;
    config.pin_reset    = -1;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = frameSize;
    config.jpeg_quality = jpegQuality;
    config.fb_count     = 1;

    esp_camera_init(&config);
  }

  camera_fb_t* capture(bool encodeBase64 = false) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) return nullptr;

    lastImageSize = fb->len;
    if (encodeBase64) {
      lastImageBase64 = base64::encode(fb->buf, fb->len);
    } else {
      lastImageBase64 = "";
    }

    esp_camera_fb_return(fb);

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
    } else {
      strcpy(isoTime, "1970-01-01T00:00:00Z"); // fallback
    }
    return fb;
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

  bool checkState() override {
    // do nothing yet
  }

  String toJson() const override {
    DynamicJsonDocument* doc = new DynamicJsonDocument(512);
    if (!doc) {
      return "{}";
    }
    
    (*doc)["name"] = name;
    (*doc)["index"] = index;
    (*doc)["type"] = type;
    (*doc)["technology"] = technology;
    (*doc)["resolution"] = frameSizeToString(frameSize);
    (*doc)["jpeg_quality"] = jpegQuality;
    (*doc)["image_size"] = lastImageSize;
    (*doc)["image_base64"] = lastImageBase64;
    (*doc)["last_updated"] = isoTime;

    String payload;
    serializeJson(*doc, payload);
    delete doc;
    return payload;
  }

  void toJsonObject(JsonObject& obj) const override {
    obj["name"] = name;
    obj["index"] = index;
    obj["type"] = type;
    obj["technology"] = technology;
    obj["resolution"] = frameSizeToString(frameSize);
    obj["jpeg_quality"] = jpegQuality;
    obj["image_size"] = lastImageSize;
    obj["image_base64"] = lastImageBase64;
    obj["last_updated"] = isoTime;
  }

private:
  String frameSizeToString(framesize_t s) const {
    switch (s) {
      case FRAMESIZE_QQVGA: return "160x120";
      case FRAMESIZE_QVGA:  return "320x240";
      case FRAMESIZE_VGA:   return "640x480";
      case FRAMESIZE_SVGA:  return "800x600";
      default:              return "custom";
    }
  }
};
}

#endif
