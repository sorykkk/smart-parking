@echo off
REM FindSpot Local Development Startup Script
echo ========================================
echo FindSpot - Local Development Setup
echo ========================================
echo.

REM Check for IP configuration
if not exist "%~dp0config.env" (
    echo WARNING: config.env not found. IP configuration may not be set.
    echo Run set-ip.ps1 to configure IP addresses.
    echo.
) else (
    echo config.env found. Running IP configuration setup...
    echo.
    powershell -ExecutionPolicy Bypass -File "%~dp0set-ip.ps1"
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: IP configuration setup failed.
        pause
        exit /b 1
    )
    echo.
)

REM Check if mosquitto is in PATH
where mosquitto >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Mosquitto not found. Please install Mosquitto MQTT broker.
    echo Download from: https://mosquitto.org/download/
    pause
    exit /b 1
)

REM Check if Python is available
where py>nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Python not found. Please install Python 3.8+
    pause
    exit /b 1
)

REM Check if Node.js is available
where node >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Node.js not found. Please install Node.js 18+
    pause
    exit /b 1
)

echo [1/3] Starting MQTT Broker...
cd findspot-backend\mosquitto
start "MQTT Broker" mosquitto -c mosquitto.conf -v
cd ..\..
timeout /t 2 >nul

echo [2/3] Starting Backend...
cd findspot-backend\flask
if not exist venv (
    echo Creating Python virtual environment...
    py -m venv venv
    call venv\Scripts\activate
    echo Installing dependencies...
    pip install -r requirements.txt
) else (
    call venv\Scripts\activate
)
start "Backend - Flask" cmd /k "venv\Scripts\activate && py app.py"
cd ..\..
timeout /t 3 >nul

echo [3/3] Starting Frontend...
cd findspot-frontend
if not exist node_modules (
    echo Installing npm dependencies...
    call npm install
)
start "Frontend - Vite" cmd /k "npm run dev"
cd ..

echo.
echo ========================================
echo All services started!
echo ========================================
