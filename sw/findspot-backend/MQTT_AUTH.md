# ESP32 MQTT Authentication - Simplified Architecture

## Overview
ESP32 devices generate their own MQTT credentials and send them to the backend during registration. The backend simply creates MQTT users with the credentials provided by ESP32.

## How It Works

### 1. ESP32 Credential Generation
- **Username**: `{MQTT_DEVICE_PREFIX}_{mac_address_without_colons}`
  - Example: For MAC `AA:BB:CC:DD:EE:FF` and prefix `esp32_dev` → username: `esp32_dev_aabbccddeeff`
- **Password**: SHA256 hash of `<mac_address><MQTT_PASS>` (truncated to 32 chars)
  - Uses ESP32's `MQTT_PASS` from env.h as secret salt
  - Generated using mbedTLS SHA256 library

### 2. Registration Flow
1. **ESP32 Boot**: Generates MQTT username and password locally
2. **MQTT Connection**: ESP32 connects to broker using generated credentials
3. **Device Registration**: ESP32 sends registration request including credentials
4. **Backend Processing**: Backend receives credentials and creates MQTT user
5. **Confirmation**: Backend confirms registration success

### 3. Implementation
- **ESP32 Functions**: In `hw/src/MQTTClient.h`
  - `generateMQTTUsername()` - Creates username from prefix + MAC
  - `generateMQTTPassword()` - Creates password using SHA256
  - `registerDevice()` - Sends credentials to backend
- **Backend Function**: In `sw/findspot-backend/flask/app.py`
  - `create_mqtt_user(username, password)` - Adds user to mosquitto passwd file
- **MQTT Config**: Mosquitto passwd file automatically updated

## Configuration

### ESP32 Configuration (Single Source)
```cpp
// Config.h
#define MQTT_DEVICE_PREFIX "esp32_dev"

// env.h  
#define MQTT_PASS "esp32dev1ved23pse"
```
All MQTT device configuration is in ESP32 only.

### Backend Configuration
```
# Backend MQTT user (separate from ESP32 devices)
MQTT_USER=flask_backend
MQTT_PASSWORD=backend_password
```
No ESP32-related MQTT configuration needed on backend.

### Backend Integration
The backend receives `mqtt_username` and `mqtt_password` from ESP32 and creates the MQTT user directly.

## Benefits
1. **Zero Manual Setup**: No need to pre-create MQTT users
2. **Secure**: Each device has unique credentials based on MAC address
3. **Deterministic**: Same MAC always generates same credentials
4. **Scalable**: Supports unlimited ESP32 devices automatically
5. **Verifiable**: ESP32 can verify credentials match backend expectations

## Security Notes
- MAC addresses are unique per device (hardware-based)
- Password generation uses cryptographic hash function
- Salt prevents rainbow table attacks
- Credentials are transmitted securely via MQTT (can add TLS later)

## Troubleshooting

### If ESP32 can't connect to MQTT:
1. Check that `MQTT_PASSWORD` environment variable is set
2. Verify ESP32 can reach the MQTT broker IP
3. Check MQTT logs for authentication failures
4. Ensure device has completed registration process

### Credential Verification:
ESP32 logs will show "✓ MQTT credentials match" if authentication is working correctly.