#ifndef DEVICE_H
#define DEVICE_H
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

class Device {
private:
    int id = -1;
    String name; // esp32_parcul_copiilor
    String location;
    double latitude;
    double longitude;
    String macAddress = "";

public:
    Device(const String& Name, const String& Location, double Latitude, double Longitude)
        : name(Name), location(Location), latitude(Latitude), longitude(Longitude) {
        macAddress = getMacAddress();
    }

    String getName() const {
        return name;
    }

    String getLocation() const {
        return location;
    }

    double getLatitude() const {
        return latitude;
    }

    double getLongitude() const {
        return longitude;
    }

    void setId(int deviceId) {
        id = deviceId;
    }

    int getId() const {
        return id;
    }

    static String getMacAddress() {
        static String cachedMac = "";
        // cache mac address
        if(cachedMac == "") 
        {
            byte mac[6];
            WiFi.macAddress(mac);
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            cachedMac = String(macStr);
        }
        return cachedMac;
    }

    String toJson() const {
        // Build registration JSON payload
        DynamicJsonDocument doc(4096);
        doc["name"] = name;
        doc["location"] = location;
        doc["latitude"] = latitude;
        doc["longitude"] = longitude;
        doc["mac_address"] = macAddress;

        String payload;
        serializeJson(doc, payload);
        return payload;
    }
};

#endif