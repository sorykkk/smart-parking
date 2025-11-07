# FindSpot - Smart Parking System

## 🚀 Complete IoT Solution

This is a smart parking system that uses ESP32 devices with ultrasonic sensors to monitor parking spot availability in real-time. The system features a **modular architecture** that allows you to deploy frontend and backend on separate servers for maximum flexibility.

### System Components:
- **ESP32 Firmware**: Publishes sensor data via MQTT with automatic device registration
- **Flask Backend**: Processes data, manages database, provides REST API
- **Svelte Frontend**: Modern web application with real-time updates
- **Mosquitto MQTT Broker**: Message broker for IoT communication  
- **Nginx**: Reverse proxy and static file serving
- **SQLite Database**: Stores parking data and device configurations

## ✨ Key Features

- 🔄 **Automatic Device Registration**: ESP32 devices automatically register with backend and receive unique IDs
- 📊 **Real-time Monitoring**: Live parking spot availability updates via MQTT and WebSocket
- 📍 **Location Tracking**: GPS coordinates and location descriptions for each device
- 🔧 **Hardware Inventory**: Backend tracks all sensors and cameras per device
- 📱 **Responsive Web App**: Modern SvelteKit application with interactive map
- 🔒 **Secure Communication**: TLS encryption, MQTT authentication
- 🌐 **RESTful API**: Complete API for device and parking management
- 🏗️ **Modular Deployment**: Deploy frontend and backend independently

## 🚀 Quick Start

Get FindSpot running in minutes on your **Raspberry Pi 5** or any server:

```bash
# Clone and configure
git clone <your-repo-url> smart-parking
cd smart-parking/sw
cp .env.example .env
# Edit .env with your SECRET_KEY and MQTT_PASSWORD

# Deploy everything
./deploy-all.sh    # Linux/Mac/Raspberry Pi
# or
deploy-all.bat     # Windows
```

**Access your system:**
- 🌐 **Frontend**: http://localhost (or http://YOUR_PI_IP)
- ⚙️ **Backend API**: http://localhost:5000  
- 📡 **MQTT Broker**: localhost:1883

**📱 For Raspberry Pi 5 deployment**, see our detailed [Raspberry Pi Setup Guide](RASPBERRY_PI_SETUP.md).

## ✨ Key Features

- 🔄 **Automatic Device Registration**: ESP32 devices automatically register with backend and receive unique IDs
- 📊 **Real-time Monitoring**: Live parking spot availability updates via MQTT and WebSocket
- 📍 **Location Tracking**: GPS coordinates and location descriptions for each device
- 🔧 **Hardware Inventory**: Backend tracks all sensors and cameras per device
- 📱 **Responsive Web App**: Modern SvelteKit application with interactive map
- 🔒 **Secure Communication**: TLS encryption, MQTT authentication
- 🌐 **RESTful API**: Complete API for device and parking management
- 🚀 **Simple Deployment**: Single-command deployment with Docker
- 🥧 **Raspberry Pi Optimized**: Resource limits and ARM architecture support

## 🔧 **Management Commands**

```bash
```bash
# Deploy everything (first time)
cd sw
./deploy-all.sh    # Linux/Mac/Raspberry Pi
deploy-all.bat     # Windows

# Start all services
docker-compose up -d

# Stop all services  
docker-compose down

# View logs (all services)
docker-compose logs -f

# View specific service logs
docker-compose logs -f flask-backend
docker-compose logs -f frontend
docker-compose logs -f mqtt-broker

# Restart specific service
docker-compose restart flask-backend

# Update and restart all
docker-compose pull && docker-compose up -d

# Remove everything (including data)
docker-compose down -v
```

### Quick Status Check
```bash
# Navigate to sw directory
cd sw

# Check all running containers
docker ps

# Check service health
curl http://localhost:5000/api/health  # Backend health
curl http://localhost                  # Frontend access
```

### Quick Status Check
```bash
# Check all running containers
docker ps

# Check service health
curl http://localhost:5000/api/health  # Backend health
curl http://localhost                  # Frontend access

# Check MQTT broker
mosquitto_pub -h localhost -t test -m "hello"  # Test MQTT
```

## 🏗️ System Architecture

The FindSpot system uses a **simple, unified architecture** with all components running together:

#### **Backend Services**
- Flask REST API (port 5000)
- MQTT Broker - Mosquitto (port 1883)  
- SQLite Database (persistent storage)

#### **Frontend Services**  
- SvelteKit web application
- Nginx web server (port 80/443)
- Real-time updates via API polling

#### **Hardware Integration**
- ESP32 devices with ultrasonic sensors
- Automatic device registration via MQTT
- Real-time data streaming to backend

---

## 📚 Documentation

- **[DEVICE_REGISTRATION.md](DEVICE_REGISTRATION.md)** - Device registration system overview
- **[ESP32_SETUP_GUIDE.md](ESP32_SETUP_GUIDE.md)** - Quick start guide for ESP32 setup
- **[DATABASE_MIGRATION.md](DATABASE_MIGRATION.md)** - Database migration guide
- **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - Complete implementation details
- **[ARCHITECTURE_DIAGRAMS.md](ARCHITECTURE_DIAGRAMS.md)** - System architecture and data flow diagrams
- **[QUICKSTART.md](QUICKSTART.md)** - Quick start for development
- **[TESTING.md](TESTING.md)** - Testing procedures
- **[DEPLOYMENT.md](DEPLOYMENT.md)** - Production deployment guide

---

## 🛠️ Setup Instructions

### 1. Raspberry Pi 5 Setup

#### Prerequisites
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER

# Install Docker Compose
sudo apt install docker-compose -y

# Install mosquitto-clients for password generation
sudo apt install mosquitto-clients -y
```

#### Clone and Configure
```bash
cd /home/pi
git clone <your-repo-url> smart-parking
cd smart-parking/sw
```

#### Configure Mosquitto Passwords
```bash
cd findspot-backend/mosquitto

# Create password for ESP32 device
mosquitto_passwd -c passwd esp32_device
# Enter: secure_device_password

# Add password for Flask backend
mosquitto_passwd passwd flask_backend
# Enter: backend_secure_password

cd ../..
```

#### Generate SSL Certificates
```bash
cd nginx
chmod +x generate-ssl.sh
./generate-ssl.sh

# For production, use Let's Encrypt:
# sudo apt install certbot
# sudo certbot certonly --standalone -d your-domain.com
cd ..
```

#### Configure Environment
```bash
# Create .env file for Docker Compose
cat > .env << EOF
SECRET_KEY=$(openssl rand -hex 32)
MQTT_BACKEND_PASSWORD=backend_secure_password
EOF
```

#### Build Frontend
```bash
cd findspot-frontend
npm install
npm run build
cd ..
```

#### Start Services
```bash
# Start all services
docker-compose up -d

# Check status
docker-compose ps

# View logs
docker-compose logs -f
```

#### Enable Port Forwarding (for internet access)
```bash
# Configure your router to forward ports:
# 443 (HTTPS) -> Raspberry Pi IP:443
# 1883 (MQTT) -> Raspberry Pi IP:1883

# Or use a service like ngrok for testing:
# ngrok http 443
```

### 2. ESP32 Setup

#### Hardware Requirements
- ESP32 Development Board
- HC-SR04 Ultrasonic Sensors (one per parking spot)
- Jumper wires
- Power supply (5V)

#### Wiring
```
Sensor 1:
- VCC -> 5V
- GND -> GND
- TRIG -> GPIO 12
- ECHO -> GPIO 13

Sensor 2:
- VCC -> 5V
- GND -> GND
- TRIG -> GPIO 14
- ECHO -> GPIO 15
```

#### Arduino IDE Setup
1. Install Arduino IDE
2. Add ESP32 board support:
   - File -> Preferences
   - Additional Board Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools -> Board -> Boards Manager -> Search "ESP32" -> Install

3. Install required libraries:
   - Sketch -> Include Library -> Manage Libraries
   - Install: `PubSubClient` by Nick O'Leary

#### Configure ESP32
Edit `hw/src/Config.h`:
```cpp
#define MQTT_BROKER "your-raspberry-pi-public-ip-or-domain"
#define MQTT_USER "esp32_device"
#define MQTT_PASSWORD "secure_device_password"
```

Edit `hw/src/env.h` (create if doesn't exist):
```cpp
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"
```

#### Upload Code
1. Connect ESP32 via USB
2. Select board: Tools -> Board -> ESP32 Dev Module
3. Select port: Tools -> Port -> (your ESP32 port)
4. Click Upload

### 3. Android App Setup

#### Prerequisites
- Node.js 18+
- Android Studio
- Java JDK 17+

#### Build Android App
```bash
cd sw/findspot-frontend

# Update API URL in .env
echo "VITE_API_URL=https://your-domain.com" > .env

# Install dependencies
npm install

# Add Android platform
npx cap add android

# Build and sync
npm run build
npx cap sync android

# Open in Android Studio
npx cap open android
```

#### Run on Device
1. Enable Developer Options on your Android phone
2. Enable USB Debugging
3. Connect phone via USB
4. In Android Studio: Run -> Run 'app'

---

## 🔧 Configuration Files

### Important Settings

#### Backend API URL
- File: `sw/findspot-frontend/.env`
- Set: `VITE_API_URL=https://your-domain.com`

#### MQTT Broker
- File: `hw/src/Config.h`
- Set: `MQTT_BROKER` to your Raspberry Pi's public IP or domain

#### Database Location
- Stored in Docker volume: `flask-data`
- Path inside container: `/app/data/findspot.db`

---

## 📱 Usage

### Access the System

1. **Web Interface** (development):
   - Local: `http://raspberry-pi-ip`
   - Public: `https://your-domain.com`

2. **Android App**:
   - Install APK from Android Studio build
   - Grant location permissions
   - App will show nearby parking spots

3. **API Endpoints**:
   ```
   GET  /api/health              - Health check
   GET  /api/locations           - All parking locations
   GET  /api/locations/nearby    - Nearby locations
   GET  /api/location/<id>       - Specific location details
   ```

4. **WebSocket**:
   - Connect to `/socket.io/`
   - Listen to `parking_update` events for real-time updates

### Add New Parking Locations

Access the database:
```bash
docker exec -it findspot-backend python

>>> from app import app, db
>>> from models import ParkingLocation
>>> with app.app_context():
...     location = ParkingLocation(
...         name="City Center Parking",
...         latitude=46.7712,
...         longitude=23.6236,
...         address="Main Street 123, Cluj-Napoca"
...     )
...     db.session.add(location)
...     db.session.commit()
```

---

## 🔍 Troubleshooting

### ESP32 Not Connecting
- Check WiFi credentials in `env.h`
- Verify MQTT broker address
- Check if port 1883 is open on Raspberry Pi

### Backend Not Receiving Data
- Check MQTT broker logs: `docker logs findspot-mosquitto`
- Verify passwords match in ESP32 and mosquitto config
- Check Flask logs: `docker logs findspot-backend`

### Android App Not Loading
- Verify API_URL in `.env` is correct
- Check if HTTPS is properly configured
- Enable CORS if needed

### Database Issues
```bash
# Reset database
docker-compose down
docker volume rm sw_flask-data
docker-compose up -d
```

---

## 🚀 Production Deployment

### Security Checklist
- [ ] Change all default passwords
- [ ] Use Let's Encrypt for SSL
- [ ] Configure firewall (ufw)
- [ ] Set up automatic backups
- [ ] Enable MQTT ACLs
- [ ] Use environment variables for secrets
- [ ] Set up monitoring (Prometheus/Grafana)

### Let's Encrypt Setup
```bash
sudo certbot certonly --standalone -d your-domain.com
sudo cp /etc/letsencrypt/live/your-domain.com/fullchain.pem sw/nginx/ssl/cert.pem
sudo cp /etc/letsencrypt/live/your-domain.com/privkey.pem sw/nginx/ssl/key.pem
```

### Automatic Renewal
```bash
sudo crontab -e
# Add line:
0 0 1 * * certbot renew --quiet && docker-compose -f /home/pi/smart-parking/sw/docker-compose.yml restart nginx
```

---

## 📊 Architecture

See `docs/ArchitectureDiagram.drawio.svg` for complete system architecture.

**Data Flow:**
1. ESP32 sensors measure distance
2. Data published to MQTT broker (topic: `sensors/DEVICE_ID/data`)
3. Flask backend subscribes and processes data
4. Data stored in SQLite database
5. WebSocket broadcasts updates to connected clients
6. Android app displays real-time parking availability

---

## 🤝 Contributing

1. Fork the repository
2. Create feature branch
3. Commit changes
4. Push to branch
5. Create Pull Request

---

## 📄 License

MIT License - feel free to use for personal or commercial projects

---

## 📧 Support

For issues or questions, please open a GitHub issue or contact the development team.
