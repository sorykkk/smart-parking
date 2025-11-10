# FindSpot Smart Parking System

A modular smart parking system with separated frontend and backend services for flexible deployment.

## 🚀 **Deployment Options**

### **Option 1: Separate Servers (Recommended for Production)**

Deploy backend and frontend on different servers for better scalability and isolation.

#### **Backend Server:**
```bash
cd findspot-backend/
cp .env.example .env
# Edit .env with production values
docker-compose up -d
```

#### **Frontend Server:**
```bash
# Configure frontend to point to backend
cp .env.example .env
# Edit .env: BACKEND_URL=https://your-backend-server.com:5000
docker-compose up -d
```

### **Option 2: Single Server (Development/Small Scale)**

Run both stacks on the same server with different ports:

```bash
# Start backend stack
cd findspot-backend/
docker-compose up -d

# Start frontend stack
cd ../
docker-compose up -d
```

### **Option 3: Full Local Development**

Run services individually for maximum development flexibility:

```bash
# Terminal 1: Backend
cd findspot-backend/flask/
python app.py

# Terminal 2: Frontend development server
cd findspot-frontend/
npm run dev

# Terminal 3: MQTT Broker (if needed)
cd findspot-backend/
docker-compose up mosquitto
```

## 🔧 **Configuration**

### **Backend Configuration** (`findspot-backend/.env`)
```bash
SECRET_KEY=your-secret-key
MQTT_PASSWORD=secure-password
BACKEND_PORT=5000
CORS_ORIGINS=https://your-frontend-domain.com
```

### **Frontend Configuration** (`.env`)
```bash
BACKEND_URL=https://your-backend-server.com:5000
BACKEND_WS_URL=wss://your-backend-server.com:5000
```

## 📡 **Network Communication**

### **Frontend → Backend**
- **REST API**: `${BACKEND_URL}/api/*`
- **WebSocket**: `${BACKEND_WS_URL}/` (real-time parking updates)

### **ESP32 Devices → Backend**
- **MQTT**: `your-backend-server:1883`

## 🌐 **Production Example**

### **Backend Server** (backend.yoursite.com)
```bash
# Backend runs on: https://backend.yoursite.com:5000
# MQTT broker on: backend.yoursite.com:1883
```

### **Frontend Server** (yoursite.com)
```bash
# Frontend served from: https://yoursite.com
# Connects to backend via: https://backend.yoursite.com:5000
```

### **ESP32 Configuration**
```cpp
// In Config.h
#define MQTT_BROKER "backend.yoursite.com"
#define MQTT_PORT 1883
```

## 📊 **Service Health**

### **Check Backend Health**
```bash
curl https://backend.yoursite.com:5000/api/health
```

### **Check Frontend**
```bash
curl https://yoursite.com
```

## 🔒 **Security Considerations**

- [ ] Configure CORS properly for production
- [ ] Use HTTPS for all external communication
- [ ] Secure MQTT with authentication
- [ ] Set strong secret keys
- [ ] Configure firewalls appropriately
- [ ] Use proper SSL certificates

## 📋 **Quick Reference**

| Service | Port | Purpose |
|---------|------|---------|
| Frontend (Nginx) | 80/443 | Web interface |
| Backend (Flask) | 5000 | REST API + WebSocket |
| MQTT Broker | 1883 | ESP32 communication |
| MQTT WebSocket | 9001 | Web MQTT clients |

## 🚀 **Getting Started**

1. **Choose your deployment strategy** (separate servers vs single server)
2. **Configure backend** (see `findspot-backend/README.md`)
3. **Configure frontend** environment variables
4. **Deploy backend first**, then frontend
5. **Configure ESP32 devices** to point to your backend MQTT broker

For detailed setup instructions, see:
- **Backend**: `findspot-backend/README.md`
- **Frontend**: `findspot-frontend/README.md` (if available)