# FindSpot - Software Components# FindSpot Smart Parking System



Local development setup for the FindSpot smart parking system.A modular smart parking system with separated frontend and backend services for flexible deployment.



## 🚀 Quick Start## 🚀 **Deployment Options**



### Automated Startup (Windows)### **Option 1: Separate Servers (Recommended for Production)**



```powershellDeploy backend and frontend on different servers for better scalability and isolation.

# Run the startup script

.\start-dev.bat#### **Backend Server:**

``````bash

cd findspot-backend/

This will:cp .env.example .env

1. Start MQTT broker# Edit .env with production values

2. Start Flask backend (with auto-setup if needed)docker-compose up -d

3. Start Vite frontend dev server```

4. Open browser to http://localhost:5173

#### **Frontend Server:**

### Manual Startup```bash

# Configure frontend to point to backend

**Terminal 1 - MQTT Broker:**cp .env.example .env

```powershell# Edit .env: BACKEND_URL=https://your-backend-server.com:5000

cd findspot-backend\mosquittodocker-compose up -d

mosquitto -c mosquitto.conf -v```

```

### **Option 2: Single Server (Development/Small Scale)**

**Terminal 2 - Backend:**

```powershellRun both stacks on the same server with different ports:

cd findspot-backend\flask

.\venv\Scripts\activate```bash

python app.py# Start backend stack

```cd findspot-backend/

docker-compose up -d

**Terminal 3 - Frontend:**

```powershell# Start frontend stack

cd findspot-frontendcd ../

npm run devdocker-compose up -d

``````



## 📁 Directory Structure### **Option 3: Full Local Development**



```Run services individually for maximum development flexibility:

sw/

├── start-dev.bat              # Automated startup script```bash

├── findspot-backend/          # Flask backend# Terminal 1: Backend

│   ├── .env.example          # Environment templatecd findspot-backend/flask/

│   ├── flask/                # Flask applicationpython app.py

│   │   ├── app.py           # Main application

│   │   ├── models.py        # Database models# Terminal 2: Frontend development server

│   │   ├── add_dummy_data.py # Test data generatorcd findspot-frontend/

│   │   └── requirements.txt  # Python dependenciesnpm run dev

│   └── mosquitto/            # MQTT broker config

│       ├── mosquitto.conf# Terminal 3: MQTT Broker (if needed)

│       └── passwd            # MQTT credentialscd findspot-backend/

└── findspot-frontend/         # Svelte frontenddocker-compose up mosquitto

    ├── src/                  # Source code```

    ├── android/              # Capacitor Android

    └── package.json          # npm dependencies## 🔧 **Configuration**

```

### **Backend Configuration** (`findspot-backend/.env`)

## 🔧 Initial Setup```bash

SECRET_KEY=your-secret-key

### 1. Backend SetupMQTT_PASSWORD=secure-password

BACKEND_PORT=5000

```powershellCORS_ORIGINS=https://your-frontend-domain.com

cd findspot-backend\flask```



# Create virtual environment### **Frontend Configuration** (`.env`)

python -m venv venv```bash

.\venv\Scripts\activateBACKEND_URL=https://your-backend-server.com:5000

BACKEND_WS_URL=wss://your-backend-server.com:5000

# Install dependencies```

pip install -r requirements.txt

## 📡 **Network Communication**

# Create .env file

cd ..### **Frontend → Backend**

cp .env.example .env- **REST API**: `${BACKEND_URL}/api/*`

# Edit .env and set SECRET_KEY and MQTT_PASSWORD- **WebSocket**: `${BACKEND_WS_URL}/` (real-time parking updates)

```

### **ESP32 Devices → Backend**

### 2. MQTT Setup- **MQTT**: `your-backend-server:1883`



```powershell## 🌐 **Production Example**

cd findspot-backend\mosquitto

### **Backend Server** (backend.yoursite.com)

# Create password file (use same password as in .env)```bash

mosquitto_passwd -c passwd flask_backend# Backend runs on: https://backend.yoursite.com:5000

# MQTT broker on: backend.yoursite.com:1883

cd ..\..```

```

### **Frontend Server** (yoursite.com)

### 3. Frontend Setup```bash

# Frontend served from: https://yoursite.com

```powershell# Connects to backend via: https://backend.yoursite.com:5000

cd findspot-frontend```



# Install dependencies### **ESP32 Configuration**

npm install```cpp

// In Config.h

# Create .env for API URL#define MQTT_BROKER "backend.yoursite.com"

echo "VITE_API_URL=http://localhost:5000" > .env#define MQTT_PORT 1883

``````



### 4. Add Test Data## 📊 **Service Health**



```powershell### **Check Backend Health**

cd findspot-backend\flask```bash

.\venv\Scripts\activatecurl https://backend.yoursite.com:5000/api/health

python add_dummy_data.py```

```

### **Check Frontend**

## 📱 Android App```bash

curl https://yoursite.com

```powershell```

cd findspot-frontend

## 🔒 **Security Considerations**

# For emulator

echo "VITE_API_URL=http://10.0.2.2:5000" > .env- [ ] Configure CORS properly for production

- [ ] Use HTTPS for all external communication

# Build and open- [ ] Secure MQTT with authentication

npm run build- [ ] Set strong secret keys

npx cap sync android- [ ] Configure firewalls appropriately

npx cap open android- [ ] Use proper SSL certificates

```

## 📋 **Quick Reference**

## 🌐 Access Points

| Service | Port | Purpose |

- **Frontend**: http://localhost:5173 (Vite dev server)|---------|------|---------|

- **Backend API**: http://localhost:5000| Frontend (Nginx) | 80/443 | Web interface |

- **MQTT Broker**: localhost:1883| Backend (Flask) | 5000 | REST API + WebSocket |

| MQTT Broker | 1883 | ESP32 communication |

## 📚 More Information| MQTT WebSocket | 9001 | Web MQTT clients |



See the main [README.md](../README.md) and [QUICKSTART.md](../QUICKSTART.md) in the root directory for detailed documentation.## 🚀 **Getting Started**


1. **Choose your deployment strategy** (separate servers vs single server)
2. **Configure backend** (see `findspot-backend/README.md`)
3. **Configure frontend** environment variables
4. **Deploy backend first**, then frontend
5. **Configure ESP32 devices** to point to your backend MQTT broker

For detailed setup instructions, see:
- **Backend**: `findspot-backend/README.md`
- **Frontend**: `findspot-frontend/README.md` (if available)