# Environment Configuration Strategy

This project uses a **dual environment setup** to support both Docker and local development:

## 🐳 **When Using Docker Compose**

```bash
# 1. Create root .env file
cp .env.example .env

# 2. Edit .env with your values
# 3. Run with Docker
docker-compose up
```

**Docker uses these variables:**
- `SECRET_KEY` - Flask security
- `MQTT_BACKEND_PASSWORD` - MQTT authentication
- Container-specific overrides (MQTT_BROKER=mosquitto, etc.)

## 💻 **When Running Locally (Development)**

```bash
# 1. Create backend .env file
cp findspot-backend/.env.example findspot-backend/.env

# 2. Edit backend .env with local values (MQTT_BROKER=localhost, etc.)
# 3. Run Flask directly
cd findspot-backend/flask
python app.py
```

**Local development uses:**
- All Flask-specific variables from `findspot-backend/.env`
- Different MQTT_BROKER (localhost vs mosquitto container)

## ⚖️ **Variable Priority**

1. **Docker environment variables** (highest priority)
2. **findspot-backend/.env** file
3. **Default values** in code (lowest priority)

## 🎯 **Key Differences by Environment**

| Variable | Docker Value | Local Dev Value | Why Different? |
|----------|-------------|----------------|----------------|
| `MQTT_BROKER` | `mosquitto` | `localhost` | Container name vs local service |
| `SECRET_KEY` | From root `.env` | From backend `.env` | Shared vs isolated |
| `CORS_ORIGINS` | `*` | `*` | Same for development |

## 🚀 **Quick Start**

### Docker (Recommended):
```bash
cp .env.example .env
docker-compose up
```

### Local Development:
```bash
cp findspot-backend/.env.example findspot-backend/.env
cd findspot-backend/flask && python app.py
```