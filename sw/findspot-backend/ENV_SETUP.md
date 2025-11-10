# Environment Configuration

This Flask backend uses environment variables for configuration. The application will automatically look for a `.env` file in the following order:

1. **Parent directory** (`sw/findspot-backend/.env`) - **Recommended**
2. **Flask directory** (`sw/findspot-backend/flask/.env`) - Fallback

## 1. Using a .env file (Recommended for Development)

1. Create file in the parent directory:

   ```bash
   # Create .env in sw/findspot-backend/
   touch .env
   ```

2. Edit the `.env` file with your specific values as in `.env.example`:
   ```bash
   # Flask Backend Environment Variables
   SECRET_KEY=<your-super-secret-key-here>
   FLASK_ENV=development
   FLASK_DEBUG=True
   
   # Database Configuration
   DATABASE_URL=sqlite:///data/findspot.db
   
   # MQTT Broker Configuration
   MQTT_BROKER=192.168.1.103
   MQTT_PORT=1883
   MQTT_USER=flask_backend
   MQTT_PASSWORD=<your-mqtt-password>
   ```

## File Structure

```
sw/findspot-backend/
├── .env                    ← Your environment file 
(recommended location)
├── .env.example            ← Template file
├── flask/
│   ├── app.py
│   └── ...
└── mosquitto/
```

## 2. Using System Environment Variables

You can set environment variables directly in your system or terminal:

### Windows (PowerShell):
```powershell
$env:SECRET_KEY="<your-secret-key>"
$env:MQTT_BROKER="192.168.1.100"
$env:MQTT_PASSWORD="<secure-password>"
python app.py
```

### Linux/Mac (Bash):
```bash
export SECRET_KEY="<your-secret-key>"
export MQTT_BROKER="192.168.1.100"
export MQTT_PASSWORD="<secure-password>"
python app.py
```

## 3. Using Docker Environment Variables

When using Docker, you can pass environment variables:

```bash
docker run -e SECRET_KEY="<your-secret-key>" -e MQTT_BROKER="192.168.1.100" your-app
```

Or use an environment file:
```bash
docker run --env-file .env <your-app>
```

## Available Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SECRET_KEY` | `dev-secret-key-change-in-production` | Flask secret key for sessions |
| `DATABASE_URL` | `sqlite:///data/findspot.db` | Database connection string |
| `MQTT_BROKER` | `mqtt` | MQTT broker hostname/IP |
| `MQTT_PORT` | `1883` | MQTT broker port |
| `MQTT_USER` | `flask_backend` | MQTT username |
| `MQTT_PASSWORD` | `backend_password` | MQTT password |
| `HOST` | `0.0.0.0` | Flask server host |
| `PORT` | `5000` | Flask server port |
| `FLASK_DEBUG` | `False` | Enable Flask debug mode |
| `CORS_ORIGINS` | `*` | Allowed CORS origins (comma-separated) |
