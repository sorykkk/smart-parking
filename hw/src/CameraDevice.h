#ifndef CAMERA_DEVICE_H
#define CAMERA_DEVICE_H

#include "esp_camera.h"
#include <Arduino.h>
#include <base64.h>  // Built-in Arduino Base64 helper

class CameraDevice {
private:
  String name;
  framesize_t frameSize;
  int jpegQuality;
  size_t lastImageSize = 0;
  String lastImageBase64;  // optional preview

public:
  CameraDevice(const String& camName, framesize_t size = FRAMESIZE_QVGA, int quality = 12)
      : name(camName), frameSize(size), jpegQuality(quality) {}

  bool begin() {
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

    return esp_camera_init(&config) == ESP_OK;
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
    return fb;
  }

  String getName() const { return name; }

  String toJSON(bool includeImage = false) const {
    String json = "{\"name\":\"" + name + "\",";
    json += "\"resolution\":\"" + frameSizeToString(frameSize) + "\",";
    json += "\"jpeg_quality\":" + String(jpegQuality) + ",";
    json += "\"last_image_size\":" + String(lastImageSize);
    if (includeImage && lastImageBase64.length() > 0) {
      json += ",\"image_base64\":\"" + lastImageBase64 + "\"";
    }
    json += "}";
    return json;
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

#endif
