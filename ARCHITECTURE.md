# FindSpot - Technical Architecture Document

## Table of Contents
1. [System Overview](#system-overview)
2. [Component Architecture](#component-architecture)
3. [Data Flow](#data-flow)
4. [Network Architecture](#network-architecture)
5. [Database Schema](#database-schema)
6. [API Specifications](#api-specifications)
7. [MQTT Topics](#mqtt-topics)
8. [Security Architecture](#security-architecture)
9. [Scalability & Performance](#scalability--performance)
10. [Monitoring & Logging](#monitoring--logging)

---

## System Overview

FindSpot is an IoT-based smart parking system that provides real-time parking availability information through an Android mobile application. The system uses ESP32 microcontrollers with ultrasonic sensors to detect vehicle presence, communicates via MQTT protocol, and delivers data through a REST API and WebSocket connections.

### Key Technologies
- **IoT Layer**: ESP32 (Arduino), HC-SR04 Ultrasonic Sensors
- **Message Broker**: Eclipse Mosquitto (MQTT 3.1.1)
- **Backend**: Python Flask, Flask-SocketIO, SQLAlchemy
- **Database**: SQLite (PostgreSQL-ready)
- **Frontend**: SvelteKit, Leaflet.js, Socket.IO
- **Mobile**: Capacitor (Android)
- **Infrastructure**: Docker, Docker Compose, Nginx
- **Protocols**: MQTT, HTTP/HTTPS, WebSocket, WSS

---

## Component Architecture

### 1. IoT Layer (ESP32 Devices)

#### Hardware Components
```
ESP32 Development Board
├── Ultrasonic Sensors (HC-SR04)
│   ├── VCC → 5V
│   ├── GND → Ground
│   ├── TRIG → GPIO Pin
│   └── ECHO → GPIO Pin
├── WiFi Module (integrated)
└── Power Supply (5V/1A minimum)
```

#### Software Architecture
```cpp
main.ino
├── WiFiManager      // WiFi connection management
├── MQTTClient       // MQTT pub/sub client
├── DistanceSensor   // Ultrasonic sensor interface
├── CameraDevice     // Optional camera support
└── Config           // Configuration parameters

Data Flow:
1. setup() → Connect WiFi → Connect MQTT → Initialize sensors
2. loop() → Read sensors → Build JSON → Publish MQTT → Wait interval
```

#### MQTT Client Features
- Auto-reconnection with exponential backoff
- QoS 1 for sensor data (at least once delivery)
- Retained messages for status
- Last Will and Testament (LWT) for disconnect detection

---

### 2. Message Broker Layer (Mosquitto)

#### Configuration
```
Mosquitto MQTT Broker
├── Port 1883 (MQTT over TCP)
│   ├── ESP32 devices
│   └── Flask backend
├── Port 9001 (MQTT over WebSocket)
│   ├── Web clients
│   └── Mobile app (optional)
├── Authentication (password file)
├── Persistence (disk-based)
└── Logging (file + stdout)
```

#### Topic Structure
```
sensors/
  └── {device_id}/
      ├── data          # Sensor readings (published by ESP32)
      ├── status        # Device status (published by ESP32)
      └── config        # Configuration updates (subscribed by ESP32)

control/
  └── {device_id}/
      ├── reboot        # Remote reboot command
      └── calibrate     # Sensor calibration command
```

---

### 3. Backend Layer (Flask Application)

#### Application Architecture
```python
Flask Application
├── app.py               # Main application
│   ├── REST API Routes
│   ├── WebSocket Handlers
│   └── MQTT Client
│
├── models.py            # SQLAlchemy ORM models
│   ├── ParkingLocation
│   ├── Device
│   ├── ParkingSpot
│   └── SensorReading
│
├── database.py          # Database initialization
│
└── Extensions
    ├── Flask-SQLAlchemy  # ORM
    ├── Flask-CORS        # Cross-origin requests
    ├── Flask-SocketIO    # WebSocket support
    └── Paho-MQTT         # MQTT client
```

#### Request Flow
```
HTTP Request → Nginx → Flask (Gunicorn) → Database → Response
MQTT Message → Mosquitto → Flask → Database → WebSocket Broadcast
WebSocket Connection → Nginx → Flask-SocketIO ↔ Client
```

#### Background Tasks
```python
# MQTT Client Loop (separate thread)
mqtt_client.loop_start()
  ├── on_connect: Subscribe to sensors/#
  ├── on_message: Process sensor data
  │   ├── Update database
  │   ├── Calculate availability
  │   └── Broadcast via WebSocket
  └── on_disconnect: Attempt reconnection
```

---

### 4. Frontend Layer (Svelte + Capacitor)

#### Component Architecture
```
SvelteKit Application
├── +page.svelte          # Main application page
│   ├── State Management
│   ├── WebSocket Connection
│   └── User Location
│
├── components/
│   ├── Map.svelte        # Leaflet map integration
│   │   ├── Marker management
│   │   ├── User location
│   │   └── Popup details
│   │
│   └── ParkingCard.svelte  # Parking location card
│       ├── Availability display
│       ├── Distance calculation
│       └── Navigation button
│
└── lib/
    ├── api.ts            # API client
    ├── websocket.ts      # WebSocket client
    └── types.ts          # TypeScript definitions
```

#### Mobile Integration (Capacitor)
```
Capacitor Android App
├── Native APIs
│   ├── Geolocation
│   ├── Status Bar
│   ├── Haptics
│   └── Keyboard
│
├── WebView
│   └── Svelte Application (bundled)
│
└── Configuration
    ├── capacitor.config.json
    └── Android manifest
```

---

### 5. Infrastructure Layer (Docker)

#### Container Architecture
```yaml
Docker Compose Stack
├── mosquitto (MQTT Broker)
│   ├── Image: eclipse-mosquitto
│   ├── Ports: 1883, 9001
│   ├── Volumes: config, data, logs
│   └── Network: findspot-network
│
├── flask-backend (API Server)
│   ├── Build: ./findspot-backend/flask
│   ├── Ports: 5000
│   ├── Volumes: database
│   ├── Depends: mosquitto
│   └── Network: findspot-network
│
└── nginx (Reverse Proxy)
    ├── Image: nginx:alpine
    ├── Ports: 80, 443
    ├── Volumes: config, ssl, frontend-build
    ├── Depends: flask-backend, mosquitto
    └── Network: findspot-network
```

#### Network Configuration
```
findspot-network (bridge)
├── mosquitto: mosquitto:1883, mosquitto:9001
├── flask-backend: flask-backend:5000
└── nginx: 0.0.0.0:80, 0.0.0.0:443

External Access:
├── Port 80 → Nginx → HTTPS redirect
├── Port 443 → Nginx → Route based on path
│   ├── / → Static files (Svelte)
│   ├── /api/* → flask-backend:5000
│   ├── /socket.io/* → flask-backend:5000 (WebSocket)
│   └── /mqtt → mosquitto:9001 (WebSocket)
└── Port 1883 → mosquitto (MQTT)
```

---

## Data Flow

### 1. Sensor Data Publication Flow

```mermaid
ESP32 → MQTT Broker → Flask Backend → Database → WebSocket → Mobile App
  ↓         ↓              ↓             ↓           ↓            ↓
 Read    Receive        Parse        Store      Broadcast    Display
Sensor   Message         JSON        Data      to Clients   on Map
```

**Detailed Steps:**

1. **ESP32 Measurement**
   ```cpp
   // Every 3 seconds
   distance = sensor.readDistance();
   isOccupied = (distance < 50cm);
   ```

2. **JSON Payload Creation**
   ```json
   {
     "device_id": "ESP32_PARKING_001",
     "timestamp": 1699123456789,
     "spots": [
       {"name": "parking-spot-1", "distance": 45.2},
       {"name": "parking-spot-2", "distance": 150.7}
     ]
   }
   ```

3. **MQTT Publish**
   ```
   Topic: sensors/ESP32_PARKING_001/data
   QoS: 1
   Retain: false
   ```

4. **Flask Processing**
   ```python
   def on_message(client, userdata, msg):
       data = json.loads(msg.payload)
       device = get_or_create_device(data['device_id'])
       for spot in data['spots']:
           update_parking_spot(device, spot)
       broadcast_update()
   ```

5. **Database Update**
   ```sql
   INSERT INTO sensor_readings (spot_id, distance_cm, is_occupied)
   VALUES (?, ?, ?);
   
   UPDATE parking_spots 
   SET is_occupied = ?, last_updated = NOW()
   WHERE id = ?;
   ```

6. **WebSocket Broadcast**
   ```python
   socketio.emit('parking_update', locations, broadcast=True)
   ```

7. **Mobile App Update**
   ```javascript
   socket.on('parking_update', (data) => {
       locations = data;
       updateMap();
   });
   ```

### 2. User Query Flow

```
Mobile App → HTTPS Request → Nginx → Flask → Database → Response
```

**Example: Nearby Parking Query**

```http
GET /api/locations/nearby?lat=46.7712&lon=23.6236&radius=5
```

```python
# Flask processes request
locations = ParkingLocation.query.all()
results = calculate_distances(locations, user_lat, user_lon)
return jsonify(results)
```

```json
Response:
[
  {
    "id": 1,
    "name": "City Center Parking",
    "latitude": 46.7712,
    "longitude": 23.6236,
    "available_spots": 5,
    "total_spots": 10,
    "distance_km": 0.5
  }
]
```

---

## Database Schema

### Entity Relationship Diagram

```
┌─────────────────────┐
│  parking_locations  │
│  ─────────────────  │
│  id (PK)            │
│  name               │
│  latitude           │
│  longitude          │
│  address            │
│  created_at         │
└──────────┬──────────┘
           │ 1:N
           │
┌──────────┴──────────┐
│   parking_spots     │
│  ─────────────────  │
│  id (PK)            │
│  location_id (FK)   │◄─────┐
│  device_id (FK)     │      │
│  spot_name          │      │
│  is_occupied        │      │
│  last_updated       │      │
│  created_at         │      │
└──────────┬──────────┘      │
           │ 1:N              │
           │                  │
┌──────────┴──────────┐      │
│  sensor_readings    │      │
│  ─────────────────  │      │
│  id (PK)            │      │
│  spot_id (FK)       │      │
│  distance_cm        │      │
│  is_occupied        │      │
│  timestamp          │      │
└─────────────────────┘      │
                             │ N:1
┌─────────────────────┐      │
│      devices        │      │
│  ─────────────────  │      │
│  id (PK)            │──────┘
│  device_id (unique) │
│  status             │
│  last_seen          │
│  created_at         │
└─────────────────────┘
```

### Table Definitions

```sql
-- Parking Locations (physical parking areas)
CREATE TABLE parking_locations (
    id INTEGER PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    latitude DECIMAL(10,7) NOT NULL,
    longitude DECIMAL(10,7) NOT NULL,
    address VARCHAR(500),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ESP32 Devices
CREATE TABLE devices (
    id INTEGER PRIMARY KEY,
    device_id VARCHAR(100) UNIQUE NOT NULL,
    status VARCHAR(50) DEFAULT 'offline',
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Individual Parking Spots (monitored by sensors)
CREATE TABLE parking_spots (
    id INTEGER PRIMARY KEY,
    location_id INTEGER NOT NULL,
    device_id INTEGER NOT NULL,
    spot_name VARCHAR(100) NOT NULL,
    is_occupied BOOLEAN DEFAULT FALSE,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (location_id) REFERENCES parking_locations(id),
    FOREIGN KEY (device_id) REFERENCES devices(id)
);

-- Historical Sensor Readings
CREATE TABLE sensor_readings (
    id INTEGER PRIMARY KEY,
    spot_id INTEGER NOT NULL,
    distance_cm FLOAT,
    is_occupied BOOLEAN,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spot_id) REFERENCES parking_spots(id)
);

-- Indexes for performance
CREATE INDEX idx_spots_location ON parking_spots(location_id);
CREATE INDEX idx_spots_device ON parking_spots(device_id);
CREATE INDEX idx_readings_spot ON sensor_readings(spot_id);
CREATE INDEX idx_readings_timestamp ON sensor_readings(timestamp);
```

---

## API Specifications

### REST API Endpoints

#### 1. Health Check
```http
GET /api/health
```
**Response:**
```json
{
  "status": "healthy",
  "mqtt_connected": true
}
```

#### 2. Get All Locations
```http
GET /api/locations
```
**Response:**
```json
[
  {
    "id": 1,
    "name": "City Center Parking",
    "latitude": 46.7712,
    "longitude": 23.6236,
    "address": "Main Street 123",
    "total_spots": 10,
    "available_spots": 7,
    "occupancy_rate": 30.0
  }
]
```

#### 3. Get Nearby Locations
```http
GET /api/locations/nearby?lat={latitude}&lon={longitude}&radius={km}
```
**Parameters:**
- `lat`: User latitude (required)
- `lon`: User longitude (required)
- `radius`: Search radius in km (optional, default: 5)

**Response:** Same as Get All Locations

#### 4. Get Location Details
```http
GET /api/location/{id}
```
**Response:**
```json
{
  "id": 1,
  "name": "City Center Parking",
  "latitude": 46.7712,
  "longitude": 23.6236,
  "address": "Main Street 123",
  "total_spots": 10,
  "available_spots": 7,
  "spots": [
    {
      "id": 1,
      "name": "parking-spot-1",
      "is_occupied": false,
      "last_updated": "2024-11-05T10:30:00Z"
    },
    {
      "id": 2,
      "name": "parking-spot-2",
      "is_occupied": true,
      "last_updated": "2024-11-05T10:29:45Z"
    }
  ]
}
```

### WebSocket Events

#### Connection
```javascript
const socket = io('https://your-domain.com');
```

#### Events from Server

**1. parking_update**
```javascript
socket.on('parking_update', (data) => {
  // data: array of parking locations with current availability
  console.log(data);
});
```

#### Events to Server

**1. request_update**
```javascript
socket.emit('request_update');
// Server responds with parking_update event
```

---

## MQTT Topics

### Topic Structure
```
sensors/
  └── {device_id}/
      ├── data      # Sensor measurements
      └── status    # Device status

control/
  └── {device_id}/
      └── config    # Configuration commands
```

### Message Formats

#### Sensor Data (sensors/{device_id}/data)
```json
{
  "device_id": "ESP32_PARKING_001",
  "timestamp": 1699123456789,
  "spots": [
    {
      "name": "parking-spot-1",
      "distance": 45.2
    },
    {
      "name": "parking-spot-2",
      "distance": 150.7
    }
  ]
}
```

#### Device Status (sensors/{device_id}/status)
```json
{
  "device": "ESP32_PARKING_001",
  "status": "online",
  "uptime": 3600000,
  "sensors": 2
}
```

---

## Security Architecture

### 1. Authentication & Authorization

**MQTT Authentication:**
- Password-based authentication
- Credentials stored in hashed format (mosquitto_passwd)
- Separate credentials for devices and backend

**API Authentication (Future):**
- JWT tokens for user sessions
- API keys for third-party integrations
- Role-based access control (RBAC)

### 2. Transport Security

**TLS/SSL:**
- All web traffic over HTTPS (TLS 1.2+)
- Nginx terminates TLS
- WebSocket Secure (WSS)
- Self-signed certs for development
- Let's Encrypt for production

**MQTT Security Options:**
- MQTT over TLS (port 8883) - optional
- Username/password authentication (enabled)
- Client certificate authentication (optional)

### 3. Network Security

**Docker Network Isolation:**
```
Internet
  ↓ (ports 80, 443, 1883)
Nginx/Mosquitto (public)
  ↓ (internal network)
Flask/Database (private)
```

**Firewall Rules:**
```bash
sudo ufw allow 22    # SSH
sudo ufw allow 80    # HTTP
sudo ufw allow 443   # HTTPS
sudo ufw allow 1883  # MQTT
sudo ufw deny 5000   # Flask (internal only)
```

### 4. Data Security

**Database:**
- SQLite file permissions (600)
- No direct external access
- Regular backups
- Sensitive data encryption (future)

**Credentials:**
- Environment variables
- Never committed to version control
- Rotation policy recommended

---

## Scalability & Performance

### Current Capacity

**Single Raspberry Pi 5:**
- 100+ concurrent WebSocket connections
- 50+ ESP32 devices
- 500+ parking spots
- 10,000+ sensor readings/hour
- ~1GB RAM usage
- ~5GB storage (with 30 days history)

### Scaling Strategies

**Horizontal Scaling:**
1. Load Balancer → Multiple Flask instances
2. Redis for session sharing
3. PostgreSQL for centralized database
4. Separate MQTT cluster

**Vertical Scaling:**
- Increase Raspberry Pi RAM
- Use SSD instead of SD card
- Optimize database queries
- Add database indexes

**Database Optimization:**
```sql
-- Partition sensor_readings by date
CREATE TABLE sensor_readings_2024_11 PARTITION OF sensor_readings
FOR VALUES FROM ('2024-11-01') TO ('2024-12-01');

-- Archive old data
DELETE FROM sensor_readings WHERE timestamp < DATE('now', '-30 days');
```

---

## Monitoring & Logging

### Log Locations

**Docker Logs:**
```bash
docker logs findspot-mosquitto
docker logs findspot-backend
docker logs findspot-nginx
```

**Application Logs:**
```
/var/lib/docker/volumes/sw_mosquitto-log/_data/mosquitto.log
/app/data/findspot.log (inside Flask container)
/var/log/nginx/access.log (inside Nginx container)
```

### Monitoring Metrics

**Key Metrics:**
- MQTT messages/second
- API response times
- WebSocket connections
- Database size
- CPU/Memory usage
- Device online/offline status

**Tools (Optional):**
- Prometheus + Grafana
- ELK Stack (Elasticsearch, Logstash, Kibana)
- Netdata
- Uptime Kuma

---

## Deployment Architecture

### Production Topology

```
Internet
  │
  ├─── Mobile App Users (HTTPS:443, WSS)
  │
  └─── ESP32 Devices (MQTT:1883)
       │
       ▼
┌──────────────────────────────┐
│    Raspberry Pi 5 Server     │
│  ┌────────────────────────┐  │
│  │   Nginx (Reverse Proxy)│  │
│  │   - TLS Termination    │  │
│  │   - Load Balancing     │  │
│  └────────┬───────────────┘  │
│           │                  │
│  ┌────────┴────────┐         │
│  │   Application   │         │
│  ├─────────────────┤         │
│  │ Flask Backend   │         │
│  │ Mosquitto MQTT  │         │
│  │ SQLite DB       │         │
│  └─────────────────┘         │
│                              │
│  Persistent Volumes:         │
│  - Database                  │
│  - MQTT Data                 │
│  - Logs                      │
└──────────────────────────────┘
```

---

**Document Version:** 1.0  
**Last Updated:** November 2025  
**Author:** FindSpot Development Team
