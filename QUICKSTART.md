# Quick Start Guide

## 🚀 Quick Setup (15 minutes)

### 1. Raspberry Pi Setup
```bash
# Clone repo
git clone <repo-url> smart-parking
cd smart-parking/sw

# Generate MQTT passwords
cd findspot-backend/mosquitto
mosquitto_passwd -c passwd esp32_device
mosquitto_passwd passwd flask_backend
cd ../..

# Generate SSL certificate
cd nginx && chmod +x generate-ssl.sh && ./generate-ssl.sh && cd ..

# Build frontend
cd findspot-frontend && npm install && npm run build && cd ..

# Start everything
docker-compose up -d
```

### 2. ESP32 Setup
1. Open `hw/src/Config.h`
2. Set your Raspberry Pi IP/domain for `MQTT_BROKER`
3. Set same password as generated above for `MQTT_PASSWORD`
4. Create `hw/src/env.h`:
   ```cpp
   #define WIFI_SSID "YourWiFi"
   #define WIFI_PASSWORD "YourPassword"
   ```
5. Upload to ESP32 using Arduino IDE

### 3. Android App
```bash
cd sw/findspot-frontend
echo "VITE_API_URL=https://your-raspberry-pi-domain" > .env
npm install
npx cap add android
npm run android
```

## 📱 Using the App

1. Open FindSpot app on Android
2. Grant location permissions
3. See nearby parking spots on map
4. Green = many free spots, Orange = some spots, Red = almost full
5. Real-time updates as ESP32 sensors detect changes

## 🔧 Troubleshooting

**ESP32 won't connect?**
- Check WiFi credentials
- Verify MQTT broker IP is accessible
- Check serial monitor for errors

**App shows no locations?**
- Access database and add location (see main README)
- Check backend logs: `docker logs findspot-backend`

**No real-time updates?**
- Check WebSocket connection in browser console
- Verify Mosquitto is running: `docker ps`

For detailed documentation, see [README.md](README.md)
