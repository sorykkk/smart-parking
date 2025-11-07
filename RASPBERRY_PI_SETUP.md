# Raspberry Pi 5 Setup Guide

This guide covers deploying FindSpot on a Raspberry Pi 5 with optimal performance and configuration.

## 🔧 Raspberry Pi 5 Prerequisites

### 1. Update System
```bash
sudo apt update && sudo apt upgrade -y
sudo reboot
```

### 2. Install Docker
```bash
# Install Docker using the official script
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add your user to docker group
sudo usermod -aG docker $USER

# Install Docker Compose
sudo apt install docker-compose -y

# Logout and login again, or run:
newgrp docker
```

### 3. Optimize Raspberry Pi 5 Settings
```bash
# Increase GPU memory split for better performance
echo 'gpu_mem=128' | sudo tee -a /boot/config.txt

# Enable container support
echo 'cgroup_enable=cpuset cgroup_enable=memory cgroup_memory=1' | sudo tee -a /boot/cmdline.txt

# Reboot to apply changes
sudo reboot
```

### 4. Install MQTT Client Tools (for testing)
```bash
sudo apt install mosquitto-clients -y
```

## 🚀 FindSpot Deployment on Pi 5

### 1. Clone Repository
```bash
cd /home/pi
git clone <your-repo-url> smart-parking
cd smart-parking/sw
```

### 2. Configure Environment
```bash
# Copy environment template
cp .env.example .env

# Edit configuration
nano .env
```

**Important Pi 5 Configuration:**

⚠️ **REQUIRED: Set secure credentials** (never use defaults!)

```bash
# Generate a secure secret key (REQUIRED)
SECRET_KEY=$(openssl rand -hex 32)
# Or use: python3 -c "import secrets; print(secrets.token_hex(32))"

# Set a secure MQTT password (REQUIRED)
MQTT_PASSWORD=your-secure-mqtt-password-here

# Set your Pi's IP address (find with: hostname -I)
PI_IP_ADDRESS=192.168.1.100  # Replace with your actual Pi IP

# Enable Pi optimizations
DOCKER_BUILDKIT=1
COMPOSE_DOCKER_CLI_BUILD=1
```

💡 **Never commit your .env file to version control!**

### 3. Deploy FindSpot
```bash
# Make deployment script executable
chmod +x deploy-all.sh

# Deploy the system
./deploy-all.sh
```

### 4. Verify Deployment
```bash
# Check all containers are running
docker ps

# Check service health
curl http://localhost:5000/api/health

# Test MQTT broker
mosquitto_pub -h localhost -p 1883 -t test -m "hello"
```

## 🌐 Network Configuration

### Find Your Pi's IP Address
```bash
# Get your Pi's IP address
hostname -I
# or
ip addr show
```

### Configure ESP32 Devices
Update your ESP32 code with your Pi's IP address:
```cpp
// In your ESP32 code
const char* mqtt_server = "192.168.1.100";  // Your Pi's IP
const int mqtt_port = 1883;
const char* mqtt_user = "flask_backend";
const char* mqtt_password = "your-secure-mqtt-password";
```

### Router Port Forwarding (Optional)
If you want external access, configure your router to forward:
- Port 80 → Pi IP:80 (Frontend)
- Port 5000 → Pi IP:5000 (Backend API)
- Port 1883 → Pi IP:1883 (MQTT - be careful with security!)

## 📊 Performance Monitoring

### System Resources
```bash
# Monitor container resource usage
docker stats

# Check Pi temperature
vcgencmd measure_temp

# Check memory usage
free -h

# Check disk usage
df -h
```

### Service Logs
```bash
# View all logs
docker-compose logs -f

# View specific service logs
docker-compose logs -f flask-backend
docker-compose logs -f mqtt-broker
docker-compose logs -f frontend
```

### Health Checks
```bash
# Backend health
curl http://localhost:5000/api/health

# Frontend access
curl http://localhost

# MQTT connectivity
mosquitto_sub -h localhost -p 1883 -t test &
mosquitto_pub -h localhost -p 1883 -t test -m "test message"
```

## 🔧 Pi 5 Optimizations

### 1. Memory Management
The docker-compose.yml includes memory limits optimized for Pi 5:
- Flask Backend: 512MB limit, 256MB reserved
- MQTT Broker: 128MB limit, 64MB reserved  
- Frontend: 256MB limit, 128MB reserved

### 2. Performance Tuning
```bash
# Create a swap file if you have limited RAM
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

### 3. Auto-start on Boot
```bash
# Enable Docker to start on boot
sudo systemctl enable docker

# Create a systemd service for FindSpot
sudo tee /etc/systemd/system/findspot.service > /dev/null <<EOF
[Unit]
Description=FindSpot Smart Parking System
Requires=docker.service
After=docker.service

[Service]
Type=oneshot
RemainAfterExit=true
WorkingDirectory=/home/pi/smart-parking
ExecStart=/usr/bin/docker-compose up -d
ExecStop=/usr/bin/docker-compose down
TimeoutStartSec=0

[Install]
WantedBy=multi-user.target
EOF

# Enable the service
sudo systemctl enable findspot.service
sudo systemctl start findspot.service
```

## 🛡️ Security Considerations

### 1. Firewall Setup
```bash
# Install UFW firewall
sudo apt install ufw -y

# Allow SSH
sudo ufw allow ssh

# Allow HTTP/HTTPS
sudo ufw allow 80
sudo ufw allow 443

# Allow backend API (be careful - consider restricting to local network)
sudo ufw allow 5000

# Allow MQTT (be very careful - consider VPN or local access only)
# sudo ufw allow 1883

# Enable firewall
sudo ufw enable
```

### 2. MQTT Security
- Change default MQTT passwords
- Consider using MQTT over TLS
- Restrict MQTT access to local network only
- Use strong authentication

### 3. Regular Updates
```bash
# Create update script
cat > update-findspot.sh << 'EOF'
#!/bin/bash
cd /home/pi/smart-parking
git pull
docker-compose pull
docker-compose up -d
EOF

chmod +x update-findspot.sh

# Add to crontab for weekly updates (optional)
# crontab -e
# Add: 0 2 * * 0 /home/pi/smart-parking/update-findspot.sh
```

## 🐛 Troubleshooting

### Container Won't Start
```bash
# Check Docker service
sudo systemctl status docker

# Check container logs
docker-compose logs [service-name]

# Restart specific service
docker-compose restart [service-name]
```

### Memory Issues
```bash
# Check memory usage
free -h
docker stats

# Clean up unused Docker resources
docker system prune -f
docker volume prune -f
```

### Network Issues
```bash
# Check if ports are listening
sudo netstat -tlnp | grep -E ':(80|443|1883|5000)'

# Test network connectivity
ping 8.8.8.8

# Check Pi's IP address
hostname -I
```

### ESP32 Connection Issues
1. Verify Pi's IP address hasn't changed
2. Check MQTT broker is running: `docker-compose logs mqtt-broker`
3. Test MQTT from another device: `mosquitto_pub -h PI_IP -p 1883 -t test -m "hello"`
4. Check firewall settings
5. Verify ESP32 WiFi connection

## 📱 Access Points

After successful deployment:
- **Frontend Web App**: http://PI_IP_ADDRESS
- **Backend API**: http://PI_IP_ADDRESS:5000
- **Health Check**: http://PI_IP_ADDRESS:5000/api/health
- **MQTT Broker**: PI_IP_ADDRESS:1883

Replace `PI_IP_ADDRESS` with your actual Raspberry Pi IP address.