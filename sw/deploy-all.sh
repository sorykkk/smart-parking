#!/bin/bash

# FindSpot Deployment Script
# Optimized for Raspberry Pi 5

set -e

echo " FindSpot Deployment (Raspberry Pi 5)"
echo "========================================"

# Check if running on ARM architecture
ARCH=$(uname -m)
if [[ $ARCH == "aarch64" || $ARCH == "armv7l" ]]; then
    echo " Detected ARM architecture: $ARCH"
    echo " Applying Raspberry Pi optimizations..."
    export DOCKER_BUILDKIT=1
    export COMPOSE_DOCKER_CLI_BUILD=1
else
    echo " Detected architecture: $ARCH"
fi

# Check if .env exists
if [ ! -f ".env" ]; then
    echo "  .env file not found. Creating from template..."
    cp .env.example .env
    echo " Please edit .env file with your configuration before continuing!"
    echo " Required: SECRET_KEY, MQTT_PASSWORD"
    echo " Optional: PI_IP_ADDRESS (your Pi's IP for ESP32 devices)"
    read -p "Press Enter after editing .env file..."
fi

# Validate required environment variables
echo " Validating required environment variables..."
source .env

if [ -z "$SECRET_KEY" ]; then
    echo " ERROR: SECRET_KEY is required but not set in .env file"
    echo " Generate one with: openssl rand -hex 32"
    exit 1
fi

if [ -z "$MQTT_PASSWORD" ]; then
    echo " ERROR: MQTT_PASSWORD is required but not set in .env file"
    echo " Set a secure password for MQTT broker authentication"
    exit 1
fi

echo " Environment variables validated"

# Get Pi IP address for user information
PI_IP=$(hostname -I | awk '{print $1}')
echo " Your Raspberry Pi IP: $PI_IP"

# Check available memory
AVAILABLE_MEMORY=$(free -m | awk 'NR==2{printf "%d", $7}')
if [ $AVAILABLE_MEMORY -lt 512 ]; then
    echo "  Warning: Low available memory (${AVAILABLE_MEMORY}MB). Consider closing other applications."
fi

# Build and start services
echo " Building and starting services..."
docker-compose down --remove-orphans 2>/dev/null || true

# Use multi-stage build for better Pi performance
echo " Building optimized containers for ARM architecture..."
docker-compose build --parallel
docker-compose up -d

# Wait for services to be ready
echo " Waiting for services to start..."
sleep 15

# Health check
echo "🩺 Running health checks..."
if curl -f "http://localhost:5000/api/health" >/dev/null 2>&1; then
    echo " Backend API is healthy"
else
    echo " Backend API health check failed"
    echo " Check logs with: docker-compose logs flask-backend"
fi

if curl -f "http://localhost" >/dev/null 2>&1; then
    echo " Frontend is accessible"
else
    echo " Frontend accessibility check failed"
    echo " Check logs with: docker-compose logs frontend"
fi

# Show service status
echo " Service Status:"
docker-compose ps

# Show resource usage
echo " Current Resource Usage:"
docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}"

echo ""
echo " Deployment complete!"
echo "========================"
echo " Frontend: http://localhost (or http://$PI_IP)"
echo " Backend API: http://localhost:5000 (or http://$PI_IP:5000)"
echo " Health Check: http://localhost:5000/api/health"
echo " MQTT Broker: localhost:1883 (or $PI_IP:1883)"
echo ""
echo " For ESP32 devices, use MQTT server: $PI_IP"
echo " To monitor: docker-compose logs -f"
echo " To stop: docker-compose down"