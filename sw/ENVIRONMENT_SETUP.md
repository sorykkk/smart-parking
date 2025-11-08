# Environment Configuration Strategy

This project uses a **simplified single environment approach** to eliminate confusion and duplicate variables.

## 🎯 **Single .env File Approach**

```bash
# Create and edit the main .env file
cp .env.example .env
nano .env
```

**All configurations use `sw/.env`:**
- Docker Compose uses these variables via `${VARIABLE}` substitution
- Local development can also use Docker for consistency
- No duplicate or conflicting environment files

## � **Production Deployment (Docker)**

```bash
# 1. Configure environment
cp .env.example .env
# Edit .env with your production values

# 2. Deploy with Docker
docker-compose up -d
```

## 💻 **Local Development Options**

### Option 1: Use Docker (Recommended)
```bash
# Same as production - use Docker for consistency
docker-compose up
```

### Option 2: Direct Flask Development
```bash
# Override MQTT_BROKER for local development
export MQTT_BROKER=localhost
cd findspot-backend/flask
python app.py
```

## ⚖️ **Variable Priority**

1. **Environment variables** (export/set commands)
2. **Docker environment section** (from .env file)
3. **Default values** in Python code

## 🎯 **Benefits of Single .env Approach**

| Benefit | Description |
|---------|-------------|
| **No Duplication** | Single source of truth for all variables |
| **No Confusion** | Only one file to edit |
| **Consistent Values** | Same variables for Docker and local dev |
| **Simple Deployment** | One command: `docker-compose up` |

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