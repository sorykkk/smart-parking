@echo off
REM FindSpot Deployment Script for Windows

echo  FindSpot Deployment
echo ======================

REM Change to sw directory where docker-compose.yml is located
REM Check if .env exists
if not exist ".env" (
    echo   .env file not found. Creating from template...
    copy .env.example .env
    echo  Please edit .env file with your configuration before continuing!
    echo  Required: SECRET_KEY, MQTT_PASSWORD
    echo  SECRET_KEY: Generate with openssl rand -hex 32
    echo  MQTT_PASSWORD: Set a secure password
    pause
    echo  Please ensure .env file is properly configured before continuing...
    pause
)

REM Build and start services
echo Building and starting services...
docker-compose down --remove-orphans 2>nul
docker-compose up --build -d

REM Wait for services to be ready
echo  Waiting for services to start...
timeout /t 10 /nobreak >nul

REM Basic health checks
echo  Services should be available at:
echo Backend API: http://localhost:5000
echo Frontend: http://localhost

REM Show service status
echo  Service Status:
docker-compose ps

echo.
echo  Deployment complete!
echo ======================
echo  Frontend: http://localhost
echo   Backend API: http://localhost:5000
echo  Health Check: http://localhost:5000/api/health
echo  MQTT Broker: localhost:1883
echo.
pause