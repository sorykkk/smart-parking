# FindSpot Backend

This directory contains the backend services for the FindSpot smart parking system, including the Flask API and MQTT broker configuration.

## Components

- **Flask Application** (`flask/`): REST API server with device management and parking data
- **MQTT Configuration** (`mosquitto/`): Mosquitto broker setup for ESP32 communication
- **Environment Configuration**: Backend-specific environment variables

## Development

For local development of the backend:

```bash
cd flask
python -m venv venv
source venv/bin/activate  # or venv\Scripts\activate on Windows
pip install -r requirements.txt
python app.py
```

## Production Deployment

The backend is deployed as part of the main FindSpot system. See the root README.md for deployment instructions. In the future it can be expanded as using separate deployment strategies with separate backend and frontend deployment.

## Configuration

Backend configuration is handled through environment variables. You can use the provided `.env.example` as a template:

```bash
# Copy the example configuration
cp .env.example .env
# Edit with your specific values
nano .env
```

Key variables:
- `SECRET_KEY`: Flask secret key for sessions
- `MQTT_PASSWORD`: Password for MQTT broker authentication
- `DATABASE_URL`: SQLite database location (default: `sqlite:///data/findspot.db`)
- `FLASK_ENV`: Environment mode (development/production)

For production deployment, these are typically set via the root `.env` file.

## API Endpoints

The Flask backend provides a REST API for device and parking management. See the main documentation for complete API reference.

```bash
# 1. Setup environment
cp .env.example .env
# Edit .env and uncomment MQTT_BROKER=localhost

# 2. Install dependencies
cd flask
pip install -r requirements.txt

# 3. Run Flask app
python app.py
```

## 🔧 **Configuration**

### **Environment Variables**

| Variable | Default | Description |
|----------|---------|-------------|
| `SECRET_KEY` | - | **Required** - Flask secret key |
| `MQTT_PASSWORD` | - | **Required** - MQTT broker password |
| `BACKEND_PORT` | `5000` | Host port for backend API |
| `FLASK_ENV` | `production` | Flask environment |
| `DATABASE_URL` | `sqlite:///data/findspot.db` | Database connection |
| `CORS_ORIGINS` | `*` | Allowed origins (set for production) |

### **Production Configuration**

For production deployment, update your `.env`:

```bash
# Security
SECRET_KEY=your-super-secure-random-key
MQTT_PASSWORD=very-secure-mqtt-password

# Environment
FLASK_ENV=production
FLASK_DEBUG=False

# CORS (specify your frontend domains)
CORS_ORIGINS=https://yourfrontend.com,https://yourdomain.com

# Port (if different)
BACKEND_PORT=8080
```

## 🌐 **External Access**

The backend exposes these ports:
- **5000** (or `BACKEND_PORT`) - Flask API Server
- **1883** - MQTT Broker
- **9001** - MQTT WebSocket

### **API Endpoints**

```bash
# Health check
GET http://your-server:5000/api/health

# Get all devices
GET http://your-server:5000/api/devices

# Get parking status
GET http://your-server:5000/api/parking/status

# WebSocket connection
ws://your-server:5000/
```

## 🔗 **Frontend Integration**

To connect a frontend running on a different server:

1. **Update CORS_ORIGINS** in backend `.env`:
   ```bash
   CORS_ORIGINS=https://your-frontend-domain.com
   ```

2. **Configure frontend** to point to backend:
   ```javascript
   const API_BASE_URL = 'https://your-backend-server.com:5000';
   const WEBSOCKET_URL = 'wss://your-backend-server.com:5000';
   ```

## 📊 **Monitoring**

### **Health Check**
```bash
curl http://localhost:5000/api/health
```

### **Logs**
```bash
# View all logs
docker-compose logs

# View specific service
docker-compose logs flask-backend
docker-compose logs mosquitto

# Follow logs
docker-compose logs -f
```

### **Database Access**
```bash
# Access SQLite database
docker-compose exec flask-backend python -c "
from database import db, init_db
from models import Device, DistanceSensor, Camera
# Your database queries here
"
```

## 🔒 **Security Considerations**

- [ ] Change default `SECRET_KEY`
- [ ] Set strong `MQTT_PASSWORD`  
- [ ] Configure specific `CORS_ORIGINS` for production
- [ ] Use HTTPS in production
- [ ] Consider using PostgreSQL for production
- [ ] Set up proper firewall rules
- [ ] Use environment-specific secrets management

## 🚢 **Deployment Examples**

### **AWS/DigitalOcean/VPS**
```bash
# Upload backend directory
scp -r findspot-backend/ user@your-server:/opt/

# SSH and deploy
ssh user@your-server
cd /opt/findspot-backend
cp .env.example .env
# Edit .env with production values
docker-compose up -d
```

### **Docker Swarm/Kubernetes**
The Docker Compose file can be adapted for orchestration platforms.

## 🔧 **Maintenance**

### **Updates**
```bash
# Pull latest images
docker-compose pull

# Restart services
docker-compose up -d

# View updated logs
docker-compose logs -f
```

### **Backup**
```bash
# Backup database
docker-compose exec flask-backend cp /app/data/findspot.db /app/data/backup-$(date +%Y%m%d).db

# Backup MQTT config
docker-compose exec mosquitto cp -r /mosquitto/config /mosquitto/backup-config
```