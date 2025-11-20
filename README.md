# FindSpot - Smart Parking System

## Local Development Setup

A smart parking system with ESP32 devices (optional), MQTT broker, Flask backend, and Svelte frontend - all running locally on your laptop for development and testing.

### System Components:
- **Flask Backend**: REST API, database, MQTT handling
- **Svelte Frontend**: Modern web app with real-time updates
- **Mosquitto MQTT Broker**: Message broker for IoT communication
- **SQLite Database**: Local data storage
- **ESP32 Firmware** (optional): Physical sensors for real parking data

## Key Features

- **Real-time Monitoring**: Live parking updates via MQTT and WebSocket
- **Location Tracking**: GPS coordinates for each parking location
- **Responsive Web App**: SvelteKit application with interactive map
- **Android App**: Capacitor-based mobile application
- **RESTful API**: Complete API for device and parking management
- **Dummy Data Support**: Test without physical hardware
- **Modular Architecture**: Backend and frontend work independently

## Quick Start

### Prerequisites
- **Python 3.8+** installed
- **Node.js 18+** and npm installed  
- **Mosquitto MQTT Broker** installed ([Download](https://mosquitto.org/download/))
- Git

### 1. Clone Repository

```powershell
git clone <your-repo-url> smart-parking
cd smart-parking
```

### 2. Setup MQTT Broker

```powershell
# Create MQTT password
cd sw\findspot-backend\mosquitto
mosquitto_passwd -c passwd flask_backend
# Enter password when prompted

# Start MQTT broker (keep this terminal open)
mosquitto -c mosquitto.conf -v
```

### 3. Setup and Run Backend

```powershell
# In a new terminal
cd sw\findspot-backend\flask

# Create virtual environment
python -m venv venv
.\venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Configure environment
cp ..\.env.example ..\.env
# Edit .env: set SECRET_KEY and MQTT_PASSWORD (same as above)

# Run backend (keep this terminal open)
python app.py
```

### 4. Add Test Data

```powershell
# In backend terminal (or new one with venv activated)
cd sw\findspot-backend\flask
.\venv\Scripts\activate
python data\add_dummy_data.py
```

### 5. Setup and Run Frontend

```powershell
# In a new terminal
cd sw\findspot-frontend

# Install dependencies
npm install

# Configure API URL
echo "VITE_API_URL=http://localhost:5000" > .env

# Run development server (keep this terminal open)
npm run dev
```

**Access your system:**
- **Frontend**: http://localhost:5173
- **Backend API**: http://localhost:5000
- **MQTT Broker**: localhost:1883


## Android App Development

### Build and Run Android App

```powershell
cd sw\findspot-frontend

# For Android Emulator
echo "VITE_API_URL=http://10.0.2.2:5000" > .env

# For Physical Device - use your laptop's IP (get with ipconfig)
# echo "VITE_API_URL=http://YOUR_LAPTOP_IP:5000" > .env

# Build and open in Android Studio
npm run build
npx cap sync android
npx cap open android
```

## Management Commands

### Starting Services

You need 3 terminals running:

**Terminal 1 - MQTT Broker:**
```powershell
cd sw\findspot-backend\mosquitto
mosquitto -c mosquitto.conf -v
```

**Terminal 2 - Backend:**
```powershell
cd sw\findspot-backend\flask
.\venv\Scripts\activate
python app.py
```

**Terminal 3 - Frontend:**
```powershell
cd sw\findspot-frontend
npm run dev
```

### Stopping Services
Press `Ctrl+C` in each terminal window.

### Database Management

```powershell
cd sw\findspot-backend\flask

# Access the database
sqlite3 data\findspot.db

# Common queries:
# .tables                          -- List all tables
# SELECT * FROM device;            -- View all devices
# SELECT * FROM distance_sensor;   -- View all sensors
# .exit                            -- Exit sqlite3
```

### Check System Status

```powershell
# Check backend health
curl http://localhost:5000/api/health

# Test MQTT
mosquitto_pub -h localhost -p 1883 -t test -m "hello"
mosquitto_sub -h localhost -p 1883 -t test -v
```

## Development Workflow

### Frontend Development

```powershell
cd sw\findspot-frontend

# Development server with hot reload
npm run dev
# Access at http://localhost:5173

# Build for production
npm run build

# Preview production build
npm run preview
```

Changes to files in `src/` will auto-reload in the browser.

### Backend Development

```powershell
cd sw\findspot-backend\flask
.\venv\Scripts\activate

# Run with auto-reload
$env:FLASK_DEBUG="1"
python app.py

# Or run normally
python app.py
```

Changes require restarting the Python process (or use Flask auto-reload).

## API Documentation

### Main Endpoints

- `GET /api/health` - Health check
- `GET /api/locations` - List all parking locations
- `GET /api/locations/<id>` - Get specific location details
- `GET /api/devices` - List all devices
- `POST /api/devices/register` - Register new device (used by ESP32)

### WebSocket Events

- `parking_update` - Real-time parking availability updates
- `device_status` - Device online/offline status

Connect to WebSocket at `http://localhost:5000/socket.io/`

## Troubleshooting

### Backend won't start
```powershell
# Check if virtual environment is activated
.\venv\Scripts\activate

# Reinstall dependencies
pip install -r requirements.txt

# Check .env file exists
cat ..\.env

# Check if port 5000 is already in use
netstat -ano | findstr :5000
```

### Frontend shows no data
```powershell
# 1. Check backend is running
curl http://localhost:5000/api/health

# 2. Add test data
cd sw\findspot-backend\flask
.\venv\Scripts\activate
python add_dummy_data.py

# 3. Check browser console for errors (F12)
```

### MQTT connection issues
```powershell
# Verify MQTT broker is running
# Check the mosquitto terminal for errors

# Test MQTT connection
mosquitto_sub -h localhost -p 1883 -t '#' -v

# Check mosquitto logs in its terminal
```

### Python module not found
```powershell
# Make sure you're in the virtual environment
cd sw\findspot-backend\flask
.\venv\Scripts\activate

# Verify packages are installed
pip list

# Reinstall if needed
pip install -r requirements.txt
```

### Android app can't connect to backend
```powershell
# 1. For emulator, use 10.0.2.2 instead of localhost
echo "VITE_API_URL=http://10.0.2.2:5000" > .env

# 2. For physical device, use your laptop's local IP
ipconfig  # Get your IP address
echo "VITE_API_URL=http://YOUR_IP:5000" > .env

# 3. Rebuild the app
npm run build
npx cap sync android

# 4. Make sure firewall allows port 5000
```

## Notes

- This setup is for **local development only**
- No SSL/HTTPS configured (HTTP only)
- Database file: `sw/findspot-backend/flask/data/findspot.db`
- MQTT broker has no encryption (for local use)
- Frontend dev server runs on port 5173 (Vite default)
- Backend runs on port 5000

### For Testing
- Backend: http://localhost:5000
- Frontend: http://localhost:5173
- MQTT: localhost:1883

### Environment Files
- Backend: `sw/findspot-backend/.env`
- Frontend: `sw/findspot-frontend/.env`
- Hardware: `hw/src/env.h`

