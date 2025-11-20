# Quick Start Guide - Local Development

## 🚀 Get Running in 5 Minutes

This guide gets you up and running with FindSpot locally on your laptop without Docker.

## Prerequisites

- **Python 3.8+** installed
- **Node.js 18+** and npm installed
- **Mosquitto MQTT Broker** installed locally
- Git installed

### Install Mosquitto (Windows)
Download and install from: https://mosquitto.org/download/
Or use Chocolatey: `choco install mosquitto`

## Step 1: Clone Repository

```powershell
git clone <your-repo-url> smart-parking
cd smart-parking
```

## Step 2: Setup MQTT Broker

```powershell
# Navigate to mosquitto config
cd sw\findspot-backend\mosquitto

# Create password file
mosquitto_passwd -c passwd flask_backend
# Enter a password when prompted

cd ..\..\..
```

Edit `sw\findspot-backend\mosquitto\mosquitto.conf` to ensure it points to the correct password file path.

## Step 3: Start MQTT Broker

```powershell
# Open a new terminal and run:
cd sw\findspot-backend\mosquitto
mosquitto -c mosquitto.conf -v
# Keep this terminal open
```

## Step 4: Setup and Run Backend

```powershell
# Open a new terminal
cd sw\findspot-backend\flask

# Create virtual environment
python -m venv venv
.\venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Create .env file
cp ..\.env.example ..\.env
# Edit .env and set SECRET_KEY and MQTT_PASSWORD (same as mosquitto passwd)

# Run backend
python app.py
# Backend runs on http://localhost:5000
# Keep this terminal open
```

## Step 5: Add Test Data

```powershell
# In the backend terminal (or new terminal with venv activated):
cd sw\findspot-backend\flask
.\venv\Scripts\activate
python data\add_dummy_data.py
```

## Step 6: Setup and Run Frontend

```powershell
# Open a new terminal
cd sw\findspot-frontend

# Install dependencies
npm install

# Create .env file for API URL
echo "VITE_API_URL=http://localhost:5000" > .env

# Run development server
npm run dev
# Frontend runs on http://localhost:5173
```

## Step 7: Access the App

Open your browser:
- **Frontend**: http://localhost:5173
- **Backend API**: http://localhost:5000/api/health

You should see a map with parking locations in Timisoara!


## 🎉 That's It!

### What's Running?

Three terminals running:
1. **MQTT Broker** (port 1883): Message broker
2. **Backend** (port 5000): REST API and WebSocket server
3. **Frontend** (port 5173): Vite dev server with hot reload

### Next Steps

#### Stop Services
Close each terminal window or press `Ctrl+C` in each.

#### Restart Services
Run the commands again in separate terminals:
- MQTT: `mosquitto -c mosquitto.conf -v`
- Backend: `python app.py`
- Frontend: `npm run dev`

## 📱 Build Android App

```powershell
cd sw\findspot-frontend

# For Android Emulator
echo "VITE_API_URL=http://10.0.2.2:5000" > .env

# For Physical Device (get your laptop IP with ipconfig)
# echo "VITE_API_URL=http://YOUR_LAPTOP_IP:5000" > .env

# Build and open in Android Studio
npm run build
npx cap sync android
npx cap open android
```

## 🐛 Troubleshooting

### "Port already in use"
```powershell
# Find what's using the port
netstat -ano | findstr :5000
netstat -ano | findstr :5173

# Then stop that process
```

### "Can't connect to MQTT"
```powershell
# Make sure mosquitto is running
# Check if you can connect:
mosquitto_sub -h localhost -p 1883 -t test -v
```

### "Module not found" in Python
```powershell
# Make sure virtual environment is activated
cd sw\findspot-backend\flask
.\venv\Scripts\activate
pip install -r requirements.txt
```

### "No data showing"
```powershell
# Run the dummy data script
cd sw\findspot-backend\flask
.\venv\Scripts\activate
python add_dummy_data.py
```

### Frontend won't start
```powershell
# Clear node_modules and reinstall
cd sw\findspot-frontend
rm -r node_modules
npm install
```

## � Development Workflow

### Backend Changes
- Edit files in `sw\findspot-backend\flask\`
- Restart: `python app.py` (or use Flask auto-reload)

### Frontend Changes  
- Edit files in `sw\findspot-frontend\src\`
- Changes auto-reload with Vite (no restart needed)

### Database Changes
```powershell
# View database
cd sw\findspot-backend\flask
sqlite3 data\findspot.db
# .tables
# SELECT * FROM device;
# .exit
```

## 📚 More Information

See the main [README.md](README.md) for:
- Detailed architecture
- API documentation  
- Hardware integration (ESP32)

