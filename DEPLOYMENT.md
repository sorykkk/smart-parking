# FindSpot Deployment Checklist

## Pre-Deployment

### Hardware
- [ ] ESP32 devices configured and tested
- [ ] Ultrasonic sensors calibrated
- [ ] Power supply stable (5V, adequate amperage)
- [ ] Sensors positioned correctly over parking spots

### Raspberry Pi 5
- [ ] OS installed and updated
- [ ] Static IP configured
- [ ] Docker and Docker Compose installed
- [ ] Adequate storage (minimum 16GB SD card recommended)
- [ ] Network configured (WiFi or Ethernet)

## Configuration

### Security
- [ ] Changed all default passwords
- [ ] Generated strong SECRET_KEY
- [ ] MQTT passwords set using mosquitto_passwd
- [ ] SSL certificates obtained (Let's Encrypt or self-signed)
- [ ] Firewall configured (ufw)
  ```bash
  sudo ufw allow 22    # SSH
  sudo ufw allow 80    # HTTP
  sudo ufw allow 443   # HTTPS
  sudo ufw allow 1883  # MQTT
  sudo ufw enable
  ```

### ESP32
- [ ] WiFi credentials configured
- [ ] MQTT broker address set to Raspberry Pi public IP/domain
- [ ] MQTT credentials match broker configuration
- [ ] Device ID unique for each ESP32
- [ ] Sensor pins correctly mapped

### Backend
- [ ] `.env` file created with production values
- [ ] Database location configured
- [ ] MQTT credentials match mosquitto config
- [ ] CORS settings appropriate for production

### Frontend
- [ ] `.env` file with correct API_URL
- [ ] Capacitor config updated with proper app ID
- [ ] Android app signed for release
- [ ] Maps API key configured (if using commercial maps)

### Networking
- [ ] Port forwarding configured on router
  - 443 -> Raspberry Pi:443 (HTTPS)
  - 1883 -> Raspberry Pi:1883 (MQTT)
- [ ] Domain name configured (if applicable)
- [ ] DNS records updated
- [ ] Let's Encrypt certificate obtained
  ```bash
  sudo certbot certonly --standalone -d your-domain.com
  ```

## Deployment Steps

### 1. Backend Deployment
```bash
cd smart-parking/sw

# Generate passwords
cd findspot-backend/mosquitto
mosquitto_passwd -c passwd esp32_device
mosquitto_passwd passwd flask_backend
cd ../..

# Setup SSL
cd nginx
./generate-ssl.sh
# Or copy Let's Encrypt certs
cd ..

# Create environment file
cp .env.example .env
nano .env  # Edit with your values

# Build and start
docker-compose build
docker-compose up -d

# Check status
docker-compose ps
docker-compose logs -f
```

### 2. Initial Database Setup
```bash
# Access Flask container
docker exec -it findspot-backend python

# Add initial parking location
>>> from app import app, db
>>> from models import ParkingLocation
>>> with app.app_context():
...     location = ParkingLocation(
...         name="Main Parking Area",
...         latitude=46.7712,
...         longitude=23.6236,
...         address="Your address here"
...     )
...     db.session.add(location)
...     db.session.commit()
...     print("Location added!")
>>> exit()
```

### 3. ESP32 Deployment
- [ ] Upload firmware to all ESP32 devices
- [ ] Test MQTT connection (check serial monitor)
- [ ] Verify data appears in backend logs
- [ ] Mount devices in parking locations

### 4. Mobile App Deployment
```bash
cd sw/findspot-frontend

# Build for production
npm install
npm run build

# Android
npx cap sync android
npx cap open android

# In Android Studio:
# - Build -> Generate Signed Bundle / APK
# - Select APK
# - Create new keystore (save credentials securely!)
# - Build Release APK
```

## Testing

### Backend Tests
- [ ] Health check: `curl https://your-domain.com/api/health`
- [ ] Locations endpoint: `curl https://your-domain.com/api/locations`
- [ ] WebSocket connection (browser console)
- [ ] MQTT broker accepting connections

### ESP32 Tests
- [ ] Serial monitor shows successful MQTT connection
- [ ] Sensor readings appear in backend logs
- [ ] Distance measurements accurate
- [ ] Automatic reconnection works

### Mobile App Tests
- [ ] App loads successfully
- [ ] Map displays correctly
- [ ] Location permissions work
- [ ] Real-time updates working
- [ ] Distance calculations accurate
- [ ] Parking status updates in real-time

## Monitoring

### Setup Monitoring
```bash
# View logs
docker-compose logs -f

# Individual service logs
docker logs findspot-backend
docker logs findspot-mosquitto
docker logs findspot-nginx

# Resource usage
docker stats

# Database size
du -sh sw/data/
```

### Automated Monitoring (Optional)
- [ ] Setup Prometheus + Grafana
- [ ] Configure alerting (email/SMS)
- [ ] Monitor MQTT message rates
- [ ] Track API response times
- [ ] Database backup schedule

## Backup & Recovery

### Backup Script
```bash
#!/bin/bash
# backup.sh

BACKUP_DIR="/home/pi/backups"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# Backup database
docker exec findspot-backend sh -c 'tar czf - /app/data' > $BACKUP_DIR/db_$DATE.tar.gz

# Backup mosquitto data
docker exec findspot-mosquitto sh -c 'tar czf - /mosquitto/data' > $BACKUP_DIR/mqtt_$DATE.tar.gz

# Keep only last 30 days
find $BACKUP_DIR -name "*.tar.gz" -mtime +30 -delete

echo "Backup completed: $DATE"
```

### Scheduled Backups
```bash
# Add to crontab
crontab -e

# Daily backup at 2 AM
0 2 * * * /home/pi/smart-parking/backup.sh
```

## Maintenance

### Regular Tasks
- [ ] Update Docker images monthly
  ```bash
  docker-compose pull
  docker-compose up -d
  ```
- [ ] Renew SSL certificates (automated with Let's Encrypt)
- [ ] Check disk space
- [ ] Review logs for errors
- [ ] Test ESP32 connections
- [ ] Verify backup integrity

### Updates
```bash
# Update backend
cd smart-parking
git pull
cd sw
docker-compose build
docker-compose up -d

# Update frontend
cd findspot-frontend
git pull
npm install
npm run build
npx cap sync android
```

## Troubleshooting

### Common Issues

**ESP32 can't connect:**
```bash
# Check MQTT broker
docker logs findspot-mosquitto

# Test MQTT locally
mosquitto_sub -h localhost -p 1883 -u esp32_device -P password -t "sensors/#" -v
```

**Backend not starting:**
```bash
# Check logs
docker logs findspot-backend

# Rebuild
docker-compose down
docker-compose build --no-cache
docker-compose up -d
```

**Database corrupted:**
```bash
# Restore from backup
docker-compose down
# Extract backup
tar xzf /home/pi/backups/db_YYYYMMDD_HHMMSS.tar.gz
docker-compose up -d
```

## Production Checklist

- [ ] All services running (`docker-compose ps`)
- [ ] HTTPS working with valid certificate
- [ ] MQTT accepting connections on port 1883
- [ ] WebSocket working for real-time updates
- [ ] At least one parking location in database
- [ ] ESP32 devices publishing data
- [ ] Mobile app can connect and display data
- [ ] Backup system configured and tested
- [ ] Monitoring in place
- [ ] Documentation updated

## Support

- Check logs first: `docker-compose logs -f`
- Review main README.md for detailed docs
- Check QUICKSTART.md for common setup issues

---

**Status:** ☐ Development | ☐ Testing | ☐ Production

**Last Updated:** _________

**Deployed By:** _________

**Notes:**
