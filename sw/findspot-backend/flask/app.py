"""
FindSpot Backend - Flask Application
Smart Parking System with MQTT, SQLite, and WebSocket support
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
from flask_socketio import SocketIO, emit
import paho.mqtt.client as mqtt
import json
import os
from datetime import datetime, timezone
import threading
from dotenv import load_dotenv
import hashlib

# Load environment variables from .env file
# First try to load from parent directory (sw/findspot-backend/)
# Then try current directory as fallback
parent_env = os.path.join(os.path.dirname(os.path.dirname(__file__)), '.env')
current_env = '.env'

if os.path.exists(parent_env):
    load_dotenv(parent_env)
    print(f"Loaded environment from: {parent_env}")
elif os.path.exists(current_env):
    load_dotenv(current_env)
    print(f"Loaded environment from: {current_env}")
else:
    print("No .env file found, using default values")

from database import db, init_db
from models import Device, DistanceSensor, Camera

app = Flask(__name__)
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY', 'dev-secret-key-change-in-production')

# Use absolute path for database to avoid issues with relative paths
db_path = os.path.join(os.path.dirname(__file__), 'data', 'findspot.db')
app.config['SQLALCHEMY_DATABASE_URI'] = os.getenv('DATABASE_URL', f'sqlite:///{db_path}')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

# Initialize extensions
cors_origins = os.getenv('CORS_ORIGINS', '*')
CORS(app, origins=cors_origins.split(',') if cors_origins != '*' else cors_origins)
socketio = SocketIO(app, cors_allowed_origins=cors_origins, async_mode='threading')
db.init_app(app)

# MQTT Configuration
MQTT_BROKER = os.getenv('MQTT_BROKER', 'localhost')
MQTT_PORT = int(os.getenv('MQTT_PORT', 1883))
MQTT_USER = os.getenv('MQTT_USER', 'flask_backend')
MQTT_PASSWORD = os.getenv('MQTT_PASSWORD', 'backend_password')
MQTT_TOPIC_SENSORS = "sensors/#"
MQTT_TOPIC_DEVICE_REGISTER = "device/register/#"
MQTT_TOPIC_SENSORS_REGISTER = "sensors/register/#"

# ESP32 MQTT Credentials Configuration
# ESP32 generates and sends its own credentials during registration

def create_mqtt_user(username, password):
    """Add MQTT user to mosquitto passwd file"""
    passwd_file = "/mosquitto/config/passwd"
    
    try:
        # Check if user already exists
        if os.path.exists(passwd_file):
            with open(passwd_file, 'r') as f:
                existing_users = f.read()
                if f"{username}:" in existing_users:
                    print(f"MQTT user {username} already exists")
                    return True
        
        # Add new user with password hash
        import subprocess
        result = subprocess.run([
            'mosquitto_passwd', '-b', passwd_file, username, password
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print(f"✓ Successfully created MQTT user: {username}")
            print(f"MQTT user {username} added to passwd file")
            
            # Restart MQTT broker to reload password file
            try:
                restart_result = subprocess.run([
                    'docker', 'restart', 'findspot-mqtt'
                ], capture_output=True, text=True, timeout=10)
                
                if restart_result.returncode == 0:
                    print(f"✓ MQTT broker restarted to reload credentials")
                else:
                    print(f"⚠ Warning: Could not restart MQTT broker: {restart_result.stderr}")
                    print(f"  User {username} will be available after manual broker restart")
            except Exception as restart_err:
                print(f"⚠ Warning: Error restarting MQTT broker: {restart_err}")
                print(f"  User {username} will be available after manual broker restart")
            
            return True
        else:
            print(f"✗ Failed to create MQTT user {username}: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"✗ Error creating MQTT user {username}: {e}")
        import traceback
        traceback.print_exc()
        return False

# Global MQTT client
mqtt_client = None


def on_connect(client, userdata, flags, rc):
    """MQTT connection callback"""
    if rc == 0:
        print(f"Connected to MQTT Broker at {MQTT_BROKER}:{MQTT_PORT}")
        client.subscribe(MQTT_TOPIC_SENSORS)
        print(f"Subscribed to topic: {MQTT_TOPIC_SENSORS}")
        client.subscribe(MQTT_TOPIC_DEVICE_REGISTER)
        print(f"Subscribed to topic: {MQTT_TOPIC_DEVICE_REGISTER}")
        client.subscribe(MQTT_TOPIC_SENSORS_REGISTER)
        print(f"Subscribed to topic: {MQTT_TOPIC_SENSORS_REGISTER}")
    else:
        print(f"Failed to connect to MQTT Broker, return code {rc}")


def on_message(client, userdata, msg):
    """MQTT message callback - processes sensor data and registration requests"""
    try:
        topic = msg.topic
        payload = json.loads(msg.payload.decode())
        print(f"Received message on {topic}: {payload}")
        
        # Process device registration
        if topic == "device/register/request":
            process_device_registration(client, payload)
        # Process sensors registration  
        elif topic == "sensors/register/request":
            process_sensors_registration(client, payload)
        # Process sensor data
        elif topic.startswith("sensors/") and "/data/" in topic:
            process_sensor_data(payload)
        elif topic.endswith("/status"):
            process_device_status(payload)
            
    except json.JSONDecodeError as e:
        print(f"✗ Failed to parse JSON: {e}")
    except Exception as e:
        print(f"✗ Error processing message: {e}")


def process_sensor_data(data):
    """Process incoming sensor data and update database"""
    with app.app_context():
        try:
            device_id = data.get('device_id') 
            sensor_index = data.get('index')
            distance = data.get('current_distance')
            is_occupied = data.get('is_occupied', False)
            
            print(f"Processing sensor data for device {device_id}, sensor {sensor_index}: distance={distance}cm, occupied={is_occupied}")
            
            # Get device by ID (since model uses id as primary key)
            device = Device.query.get(device_id)
            if not device:
                print(f"Device {device_id} not found")
                return
                
            # Update device last seen
            device.last_seen = datetime.now(timezone.utc)
            device.status = 'online'
            
            # Get sensor by device_id and index
            sensor = DistanceSensor.query.filter_by(device_id=device.id, index=sensor_index).first()
            if sensor:
                # Update sensor data
                sensor.current_distance = distance
                sensor.is_occupied = is_occupied
                sensor.last_updated = datetime.now(timezone.utc)

            db.session.commit()
            
            # Broadcast update to connected clients via WebSocket
            broadcast_parking_update()
            
        except Exception as e:
            db.session.rollback()
            print(f"Error processing sensor data: {e}")


def process_device_registration(mqtt_client, data):
    """Process device registration request via MQTT (Phase 1: Device only)"""
    with app.app_context():
        try:
            mac_address = data.get('mac_address')
            name = data.get('name')
            location = data.get('location')
            latitude = data.get('latitude')
            longitude = data.get('longitude')
            
            # ESP32 sends its own generated credentials
            mqtt_username = data.get('mqtt_username')
            mqtt_password = data.get('mqtt_password')
            
            if not mac_address:
                print("Registration failed: MAC address required")
                return
            
            if not mqtt_username or not mqtt_password:
                print("Registration failed: MQTT credentials required")
                return
            
            print(f"Processing device registration for MAC: {mac_address}")
            print(f"ESP32 MQTT username: {mqtt_username}")
            
            # Check if device already exists
            device = Device.query.filter_by(mac_address=mac_address).first()
            
            if device:
                print(f"Device exists: ID {device.id}, updating...")
                # Update existing device
                device.name = name
                device.location = location
                device.latitude = latitude
                device.longitude = longitude
                device.status = 'registered'
                device.registered_at = datetime.now(timezone.utc)
                device.last_seen = datetime.now(timezone.utc)

                device_id = device.id
                is_new = False
            else:
                print(f"Creating new device")
                
                device = Device(
                    mac_address=mac_address,
                    name=name,
                    location=location,
                    latitude=latitude,
                    longitude=longitude,
                    status='registered',
                    registered_at=datetime.now(timezone.utc)
                )
                db.session.add(device)
                db.session.flush()  # Get the auto-generated ID
                device_id = device.id
                is_new = True
            
            db.session.commit()
            
            print(f"Device registered: {device_id}")
            
            # Create MQTT user with credentials provided by ESP32
            mqtt_created = create_mqtt_user(mqtt_username, mqtt_password)
            
            if mqtt_created:
                print(f"MQTT credentials created for device {mac_address}")
            else:
                print(f"Warning: Failed to create MQTT credentials for device {mac_address}")
            
            # Send response back to device via MQTT
            response_topic = f"device/register/{mac_address}/response"
            response_payload = {
                'status': 'registered',
                'id': device_id,
                'mac_address': mac_address,
                'message': 'Device registered successfully' if is_new else 'Device updated successfully'
            }
            
            mqtt_client.publish(response_topic, json.dumps(response_payload))
            print(f"Sent device registration response to {response_topic}")
            
        except Exception as e:
            db.session.rollback()
            print(f"Error processing device registration: {e}")
            
            # Send error response
            if 'mac_address' in locals():
                response_topic = f"device/register/{mac_address}/response"
                response_payload = {
                    'status': 'error',
                    'message': str(e)
                }
                mqtt_client.publish(response_topic, json.dumps(response_payload))


def process_sensors_registration(mqtt_client, data):
    """Process sensors registration request via MQTT (Phase 2: Sensors)"""
    with app.app_context():
        try:
            device_id = data.get('device_id')  # ESP32 sends device_id
            
            if not device_id:
                print("Sensors registration failed: Device ID required")
                return
                
            print(f"Processing sensors registration for device: {device_id}")
            
            # Get device by ID (not device_id since model uses id as primary key)
            device = Device.query.get(device_id)
            if not device:
                print(f"Device {device_id} not found")
                return
            
            # Clear old sensors and cameras
            DistanceSensor.query.filter_by(device_id=device.id).delete()
            Camera.query.filter_by(device_id=device.id).delete()
            
            sensors_count = 0
            cameras_count = 0
            
            # Register distance sensors
            if 'distance' in data:
                sensors_data = data['distance']
                for sensor_data in sensors_data:
                    sensor = DistanceSensor(
                        device_id=device.id,
                        name=sensor_data.get('name'),
                        index=sensor_data.get('index'),
                        technology=sensor_data.get('technology', 'ultrasonic'),
                        trigger_pin=sensor_data.get('trigger_pin'),
                        echo_pin=sensor_data.get('echo_pin'),
                        is_occupied=sensor_data.get('is_occupied', False),
                        current_distance=sensor_data.get('current_distance')
                    )
                    db.session.add(sensor)
                    sensors_count += 1
            
            # Register cameras
            if 'camera' in data:
                cameras_data = data['camera']
                for camera_data in cameras_data:
                    camera = Camera(
                        device_id=device.id,
                        name=camera_data.get('name'),
                        index=camera_data.get('index'),
                        technology=camera_data.get('technology', 'OV2640'),
                        resolution=camera_data.get('resolution'),
                        jpeg_quality=camera_data.get('quality')
                    )
                    db.session.add(camera)
                    cameras_count += 1
            
            db.session.commit()
            
            print(f"Sensors registered for {device_id}: {sensors_count} sensors and {cameras_count} cameras")
            
            # Send response back to device via MQTT  
            response_topic = f"device/register/{device.mac_address}/response"
            response_payload = {
                'status': 'registered',
                'sensors_registered': sensors_count,
                'cameras_registered': cameras_count,
                'message': 'Sensors registered successfully'
            }
            
            mqtt_client.publish(response_topic, json.dumps(response_payload))
            print(f"Sent sensors registration response to {response_topic}")
            
        except Exception as e:
            db.session.rollback()
            print(f"Error processing sensors registration: {e}")
            
            # Send error response
            if 'device' in locals() and device:
                response_topic = f"device/register/{device.mac_address}/response"
                response_payload = {
                    'status': 'error',
                    'message': str(e)
                }
                mqtt_client.publish(response_topic, json.dumps(response_payload))


def process_device_status(data):
    """Process device status updates"""
    with app.app_context():
        try:
            device_id = data.get('device')
            status = data.get('status')
            
            device = Device.query.filter_by(device_id=device_id).first()
            if device:
                device.status = status
                device.last_seen = datetime.now(timezone.utc)
                db.session.commit()
                print(f"Updated device {device_id} status to {status}")
        except Exception as e:
            db.session.rollback()
            print(f"Error updating device status: {e}")


def broadcast_parking_update():
    """Broadcast parking updates to all connected WebSocket clients"""
    try:
        parking_data = get_all_parking_data()
        socketio.emit('parking_update', parking_data, namespace='/', broadcast=True)
        print(f"Broadcasted parking update to WebSocket clients")
    except Exception as e:
        print(f"Error broadcasting update: {e}")


def init_mqtt():
    """Initialize MQTT client"""
    global mqtt_client
    mqtt_client = mqtt.Client()
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        print("MQTT Client initialized and loop started")
    except Exception as e:
        print(f"Failed to connect to MQTT broker: {e}")


# REST API Endpoints

@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'mqtt_connected': mqtt_client.is_connected() if mqtt_client else False
    })


# Device Registration Endpoints

@app.route('/api/device/pre-register', methods=['POST'])
def pre_register_device():
    """
    Pre-register a device and generate a unique device ID.
    ESP32 calls this on first boot to get its ID.
    
    Expected JSON:
    {
        "mac_address": "AA:BB:CC:DD:EE:FF",
        "name": "ESP32 Device",
        "location": "Building A",
        "latitude": 45.7489,
        "longitude": 21.2087
    }
    
    Returns:
    {
        "id": 1,
        "status": "pre-registered"
    }
    """
    try:
        data = request.get_json()
        mac_address = data.get('mac_address')
        name = data.get('name', 'ESP32 Device')
        location = data.get('location', 'Unknown Location')
        latitude = data.get('latitude', 0.0)
        longitude = data.get('longitude', 0.0)
        
        if not mac_address:
            return jsonify({'error': 'MAC address is required'}), 400
        
        # Check if device with this MAC already exists
        existing_device = Device.query.filter_by(mac_address=mac_address).first()
        if existing_device:
            return jsonify({
                'id': existing_device.id,
                'status': existing_device.status,
                'message': 'Device already exists'
            }), 200
        
        # Create pre-registered device
        new_device = Device(
            mac_address=mac_address,
            name=name,
            location=location,
            latitude=latitude,
            longitude=longitude,
            status='pre-registered'
        )
        
        db.session.add(new_device)
        db.session.commit()
        
        print(f"Pre-registered device: {new_device.id} with MAC: {mac_address}")
        
        return jsonify({
            'id': new_device.id,
            'status': 'pre-registered',
            'message': 'Device pre-registered successfully'
        }), 201
        
    except Exception as e:
        db.session.rollback()
        print(f"Error pre-registering device: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/register', methods=['POST'])
def register_device():
    """
    Complete device registration with full information.
    ESP32 calls this after getting device_id from pre-register.
    
    Expected JSON:
    {
        "id": 1,
        "name": "Parking Lot A - Device 1",
        "location": "Building A, Level 2, North Wing",
        "latitude": 45.7489,
        "longitude": 21.2087,
        "sensors": [
            {
                "name": "parking-spot-1",
                "index": 0,
                "trigger_pin": 12,
                "echo_pin": 13
            }
        ],
        "cameras": [
            {
                "name": "esp-cam-1",
                "index": 0,
                "technology": "OV2640",
                "resolution": "QVGA",
                "jpeg_quality": 10
            }
        ]
    }
    
    Returns:
    {
        "status": "registered",
        "id": 1,
        "sensors_registered": 2,
        "cameras_registered": 1
    }
    """
    try:
        data = request.get_json()
        device_id = data.get('id')
        
        if not device_id:
            return jsonify({'error': 'device id is required'}), 400
        
        # Find the device by ID
        device = Device.query.get(device_id)
        if not device:
            return jsonify({'error': 'Device not found. Please pre-register first.'}), 404
        
        # Update device information
        device.name = data.get('name')
        device.location = data.get('location')
        device.latitude = data.get('latitude')
        device.longitude = data.get('longitude')
        device.status = 'registered'
        device.registered_at = datetime.now(timezone.utc)
        
        # Clear existing sensors and cameras
        DistanceSensor.query.filter_by(device_id=device.id).delete()
        Camera.query.filter_by(device_id=device.id).delete()
        
        # Register sensors
        sensors_data = data.get('sensors', [])
        sensors_count = 0
        
        for sensor_data in sensors_data:
            sensor = DistanceSensor(
                device_id=device.id,
                name=sensor_data.get('name'),
                index=sensor_data.get('index', sensors_count),
                technology=sensor_data.get('technology', 'ultrasonic'),
                trigger_pin=sensor_data.get('trigger_pin'),
                echo_pin=sensor_data.get('echo_pin')
            )
            db.session.add(sensor)
            sensors_count += 1
        
        # Register cameras
        cameras_data = data.get('cameras', [])
        cameras_count = 0
        
        for camera_data in cameras_data:
            camera = Camera(
                device_id=device.id,
                name=camera_data.get('name'),
                index=camera_data.get('index', cameras_count),
                technology=camera_data.get('technology', 'OV2640'),
                resolution=camera_data.get('resolution'),
                jpeg_quality=camera_data.get('jpeg_quality')
            )
            db.session.add(camera)
            cameras_count += 1
        
        db.session.commit()
        
        print(f"Registered device: {device_id} with {sensors_count} sensors and {cameras_count} cameras")
        
        return jsonify({
            'status': 'registered',
            'id': device_id,
            'sensors_registered': sensors_count,
            'cameras_registered': cameras_count,
            'message': 'Device registered successfully'
        }), 200
        
    except Exception as e:
        db.session.rollback()
        print(f"Error registering device: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/<int:device_id>', methods=['GET'])
def get_device_info(device_id):
    """Get detailed information about a specific device"""
    device = Device.query.get_or_404(device_id)
    
    sensors = DistanceSensor.query.filter_by(device_id=device.id).all()
    cameras = Camera.query.filter_by(device_id=device.id).all()
    
    return jsonify({
        'id': device.id,
        'name': device.name,
        'location': device.location,
        'latitude': float(device.latitude) if device.latitude else None,
        'longitude': float(device.longitude) if device.longitude else None,
        'status': device.status,
        'mac_address': device.mac_address,
        'last_seen': device.last_seen.isoformat() if device.last_seen else None,
        'created_at': device.created_at.isoformat() if device.created_at else None,
        'registered_at': device.registered_at.isoformat() if device.registered_at else None,
        'sensors': [{
            'name': s.name,
            'index': s.index,
            'type': s.type,
            'technology': s.technology,
            'trigger_pin': s.trigger_pin,
            'echo_pin': s.echo_pin,
            'current_distance': s.current_distance,
            'is_occupied': s.is_occupied,
            'last_updated': s.last_updated.isoformat() if s.last_updated else None
        } for s in sensors],
        'cameras': [{
            'name': c.name,
            'index': c.index,
            'type': c.type,
            'technology': c.technology,
            'resolution': c.resolution,
            'jpeg_quality': c.jpeg_quality,
            'last_updated': c.last_updated.isoformat() if c.last_updated else None
        } for c in cameras]
    })


@app.route('/api/devices', methods=['GET'])
def get_all_devices():
    """Get list of all registered devices"""
    devices = Device.query.all()
    
    return jsonify([{
        'id': d.id,
        'name': d.name,
        'location': d.location,
        'latitude': float(d.latitude) if d.latitude else None,
        'longitude': float(d.longitude) if d.longitude else None,
        'status': d.status,
        'mac_address': d.mac_address,
        'last_seen': d.last_seen.isoformat() if d.last_seen else None,
        'sensors_count': DistanceSensor.query.filter_by(device_id=d.id).count(),
        'cameras_count': Camera.query.filter_by(device_id=d.id).count()
    } for d in devices])


@app.route('/api/parking/status', methods=['GET'])
def get_parking_status():
    """Get current parking status from all devices"""
    try:
        devices = Device.query.filter(Device.status.in_(['registered', 'online'])).all()
        parking_data = []
        
        for device in devices:
            sensors = DistanceSensor.query.filter_by(device_id=device.id).all()
            device_data = {
                'id': device.id,
                'name': device.name,
                'location': device.location,
                'latitude': float(device.latitude) if device.latitude else None,
                'longitude': float(device.longitude) if device.longitude else None,
                'status': device.status,
                'last_seen': device.last_seen.isoformat() if device.last_seen else None,
                'parking_spots': []
            }
            
            for sensor in sensors:
                spot_data = {
                    'name': sensor.name,
                    'index': sensor.index,
                    'is_occupied': sensor.is_occupied,
                    'current_distance': sensor.current_distance,
                    'last_updated': sensor.last_updated.isoformat() if sensor.last_updated else None
                }
                device_data['parking_spots'].append(spot_data)
            
            parking_data.append(device_data)
        
        return jsonify(parking_data)
    except Exception as e:
        print(f"Error getting parking status: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/api/parking/nearby', methods=['GET'])
def get_nearby_parking():
    """Get nearby parking based on user coordinates"""
    try:
        lat = float(request.args.get('lat', 0))
        lon = float(request.args.get('lon', 0))
        radius_km = float(request.args.get('radius', 5))
        
        # For now, return all devices (implement distance calculation later)
        devices = Device.query.filter(Device.status.in_(['registered', 'online'])).all()
        parking_data = []
        
        for device in devices:
            if device.latitude and device.longitude:
                sensors = DistanceSensor.query.filter_by(device_id=device.id).all()
                total_spots = len(sensors)
                available_spots = sum(1 for s in sensors if not s.is_occupied)
                
                device_data = {
                    'id': device.id,
                    'name': device.name,
                    'location': device.location,
                    'latitude': float(device.latitude),
                    'longitude': float(device.longitude),
                    'total_spots': total_spots,
                    'available_spots': available_spots,
                    'occupancy_rate': round((total_spots - available_spots) / total_spots * 100, 1) if total_spots > 0 else 0
                }
                parking_data.append(device_data)
        
        return jsonify(parking_data)
    except ValueError:
        return jsonify({'error': 'Invalid coordinates'}), 400
    except Exception as e:
        return jsonify({'error': str(e)}), 500


@app.route('/api/device/<int:device_id>/sensors', methods=['GET'])
def get_device_sensors(device_id):
    """Get detailed sensor information for a specific device"""
    device = Device.query.get_or_404(device_id)
    
    sensors = DistanceSensor.query.filter_by(device_id=device.id).all()
    total_spots = len(sensors)
    available_spots = sum(1 for s in sensors if not s.is_occupied)
    
    return jsonify({
        'id': device.id,
        'name': device.name,
        'location': device.location,
        'latitude': float(device.latitude) if device.latitude else None,
        'longitude': float(device.longitude) if device.longitude else None,
        'total_spots': total_spots,
        'available_spots': available_spots,
        'sensors': [{
            'name': s.name,
            'index': s.index,
            'type': s.type,
            'technology': s.technology,
            'trigger_pin': s.trigger_pin,
            'echo_pin': s.echo_pin,
            'current_distance': s.current_distance,
            'is_occupied': s.is_occupied,
            'last_updated': s.last_updated.isoformat() if s.last_updated else None
        } for s in sensors]
    })


def get_all_parking_data():
    """Helper function to get all parking data from devices"""
    devices = Device.query.filter(Device.status.in_(['registered', 'online'])).all()
    result = []
    
    for device in devices:
        sensors = DistanceSensor.query.filter_by(device_id=device.id).all()
        total_spots = len(sensors)
        available_spots = sum(1 for s in sensors if not s.is_occupied)
        
        device_data = {
            'id': device.id,
            'name': device.name,
            'location': device.location,
            'latitude': float(device.latitude) if device.latitude else None,
            'longitude': float(device.longitude) if device.longitude else None,
            'status': device.status,
            'last_seen': device.last_seen.isoformat() if device.last_seen else None,
            'total_spots': total_spots,
            'available_spots': available_spots,
            'occupancy_rate': round((total_spots - available_spots) / total_spots * 100, 1) if total_spots > 0 else 0,
            'sensors': [{
                'name': s.name,
                'index': s.index,
                'is_occupied': s.is_occupied,
                'current_distance': s.current_distance,
                'last_updated': s.last_updated.isoformat() if s.last_updated else None
            } for s in sensors]
        }
        result.append(device_data)
    
    return result


# WebSocket Events

@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    print(f"Client connected: {request.sid}")
    # Send current parking status to newly connected client
    parking_data = get_all_parking_data()
    emit('parking_update', parking_data)


@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print(f"🔌 Client disconnected: {request.sid}")


@socketio.on('request_update')
def handle_update_request():
    """Handle explicit update request from client"""
    parking_data = get_all_parking_data()
    emit('parking_update', parking_data)


# Camera Endpoints

@app.route('/api/device/<int:device_id>/cameras', methods=['GET'])
def get_device_cameras(device_id):
    """Get camera information for a specific device"""
    device = Device.query.get_or_404(device_id)
    
    cameras = Camera.query.filter_by(device_id=device.id).all()
    
    return jsonify({
        'id': device.id,
        'name': device.name,
        'cameras': [{
            'name': c.name,
            'index': c.index,
            'type': c.type,
            'technology': c.technology,
            'resolution': c.resolution,
            'jpeg_quality': c.jpeg_quality,
            'image_size': c.image_size,
            'has_image': bool(c.image_base64),
            'plate_registered': c.plate_registered,
            'plate_number': c.plate_number,
            'last_updated': c.last_updated.isoformat() if c.last_updated else None
        } for c in cameras]
    })


@app.route('/api/device/<int:device_id>/camera/<int:camera_index>/image', methods=['GET'])
def get_camera_image(device_id, camera_index):
    """Get the latest image from a specific camera"""
    device = Device.query.get_or_404(device_id)
    camera = Camera.query.filter_by(device_id=device.id, index=camera_index).first_or_404()
    
    if not camera.image_base64:
        return jsonify({'error': 'No image available'}), 404
        
    return jsonify({
        'device_id': device.id,
        'camera_name': camera.name,
        'camera_index': camera.index,
        'image_base64': camera.image_base64,
        'image_size': camera.image_size,
        'last_updated': camera.last_updated.isoformat() if camera.last_updated else None
    })


if __name__ == '__main__':
    # Create data directory if it doesn't exist
    os.makedirs('data', exist_ok=True)
    
    # Initialize database
    with app.app_context():
        init_db()
    
    # Initialize MQTT client
    init_mqtt()
    
    # Get server configuration from environment
    host = os.getenv('HOST', '0.0.0.0')
    port = int(os.getenv('PORT', 5000))
    debug = os.getenv('FLASK_DEBUG', 'False').lower() == 'true'
    
    # Run Flask-SocketIO server
    print("Starting FindSpot Backend Server...")
    print(f"Server will run on http://{host}:{port}")
    print(f"MQTT Broker: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"Database: {app.config['SQLALCHEMY_DATABASE_URI']}")
    
    socketio.run(app, host=host, port=port, debug=debug, allow_unsafe_werkzeug=True)
