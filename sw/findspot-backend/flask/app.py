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

# Device timeout configuration (in seconds)
DEVICE_TIMEOUT = int(os.getenv('DEVICE_TIMEOUT', 60))  # Default: 60 seconds

# ESP32 MQTT Credentials Configuration
# ESP32 generates and sends its own credentials during registration

def is_device_online(device):
    """Check if device is considered online based on last_seen timestamp"""
    if not device.last_seen:
        return False
    
    # Ensure last_seen is timezone-aware
    last_seen = device.last_seen
    if last_seen.tzinfo is None:
        last_seen = last_seen.replace(tzinfo=timezone.utc)
    
    time_since_last_seen = (datetime.now(timezone.utc) - last_seen).total_seconds()
    return time_since_last_seen <= DEVICE_TIMEOUT

def create_mqtt_user(username, password):
    """Add MQTT user to mosquitto passwd file - Local Development Version"""
    # Update path for local development
    passwd_file = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'mosquitto', 'passwd')
    
    try:
        # Check if user already exists
        if os.path.exists(passwd_file):
            with open(passwd_file, 'r') as f:
                existing_users = f.read()
                if f"{username}:" in existing_users:
                    print(f"MQTT user {username} already exists")
                    return True
        
        # Add new user with password hash using mosquitto_passwd
        import subprocess
        
        # For local development, try to find mosquitto_passwd
        mosquitto_cmd = 'mosquitto_passwd'
        
        # Check if mosquitto_passwd is available
        try:
            subprocess.run([mosquitto_cmd, '--help'], capture_output=True, check=True)
        except (subprocess.CalledProcessError, FileNotFoundError):
            # Try Windows default path
            mosquitto_cmd = r'C:\Program Files\mosquitto\mosquitto_passwd.exe'
            if not os.path.exists(mosquitto_cmd):
                print(f"⚠ Warning: mosquitto_passwd not found. User {username} not added to MQTT.")
                return False
        
        result = subprocess.run([
            mosquitto_cmd, '-b', passwd_file, username, password
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print(f"Successfully created MQTT user: {username}")
            print(f"MQTT user {username} added to passwd file")
            print(f"  Note: Restart mosquitto manually to reload credentials")
            return True
        else:
            print(f"X Failed to create MQTT user {username}: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"Error creating MQTT user {username}: {e}")
        return False

# Global MQTT client
mqtt_client = None


def on_connect(client, userdata, flags, rc):
    """MQTT connection callback"""
    if rc == 0:
        print(f"Connected to MQTT Broker at {MQTT_BROKER}:{MQTT_PORT}")
        client.subscribe("device/+/sensors/+", qos=0)
        client.subscribe("device/+/status", qos=0)
    else:
        print(f"Failed to connect to MQTT Broker, return code {rc}")


def on_message(client, userdata, msg):
    """MQTT message callback - processes sensor data and auto-registers sensors"""
    try:
        topic = msg.topic
        payload = json.loads(msg.payload.decode())
        
        # Handle individual sensor data: device/{device_id}/sensors/{sensor_index}
        if topic.startswith("device/") and "/sensors/" in topic:
            parts = topic.split('/')
            if len(parts) == 4:
                device_id = int(parts[1])
                sensor_index = int(parts[3])
                process_single_sensor_data(device_id, sensor_index, payload)
        # Handle device status updates: device/{device_id}/status
        elif topic.startswith("device/") and topic.endswith("/status"):
            parts = topic.split('/')
            if len(parts) == 3:
                device_id = int(parts[1])
                process_device_status(device_id, payload)
            
    except json.JSONDecodeError as e:
        print(f"Failed to parse JSON from MQTT: {e}")
    except Exception as e:
        print(f"Error processing MQTT message: {e}")
        import traceback
        traceback.print_exc()


def process_single_sensor_data(device_id, sensor_index, data):
    """Process individual sensor data update from ESP32"""
    ctx = app.app_context()
    ctx.push()
    try:
        # Validate device exists
        device = Device.query.get(device_id)
        if not device:
            return
        
        # Update device status and last_seen
        device.last_seen = datetime.now(timezone.utc)
        device.status = 'online'
        
        sensor_name = data.get('name', f'sensor_{sensor_index}')
        distance = data.get('current_distance')
        is_occupied = data.get('is_occupied', False)
        trigger_pin = data.get('trigger_pin')
        echo_pin = data.get('echo_pin')
        
        if distance is None:
            db.session.commit()
            return
        
        # Find or create sensor
        sensor = DistanceSensor.query.filter_by(
            device_id=device_id, 
            index=sensor_index
        ).first()
        
        if not sensor:
            # Auto-register new sensor
            sensor = DistanceSensor(
                device_id=device_id,
                name=sensor_name,
                index=sensor_index,
                type=data.get('type', 'distance'),
                technology=data.get('technology', 'ultrasonic'),
                trigger_pin=trigger_pin if trigger_pin is not None else 0,
                echo_pin=echo_pin if echo_pin is not None else 0,
                current_distance=distance,
                is_occupied=is_occupied
            )
            db.session.add(sensor)
            db.session.flush()
            
            # Notify frontend about new sensor
            socketio.emit('sensor_registered', {
                'device_id': device_id,
                'sensor_name': sensor_name,
                'sensor_index': sensor_index
            })
        else:
            # Update existing sensor
            sensor.current_distance = distance
            sensor.is_occupied = is_occupied
            sensor.last_updated = datetime.now(timezone.utc)
        
        # Commit all changes to database
        db.session.commit()
        
        # Real-time update to frontend
        socketio.emit('sensor_update', {
            'device_id': device_id,
            'sensor_index': sensor_index,
            'sensor_name': sensor_name,
            'distance': distance,
            'occupied': is_occupied,
            'timestamp': datetime.now(timezone.utc).isoformat()
        })
        
        # Notify frontend about device update
        socketio.emit('device_update', {
            'device_id': device_id,
            'status': 'online',
            'last_seen': device.last_seen.isoformat()
        })
        
        # Send full parking update to ensure frontend has latest data
        parking_data = get_all_parking_data()
        socketio.emit('parking_update', parking_data)
        
    except Exception as e:
        db.session.rollback()
        print(f"Error processing sensor data: {e}")
        import traceback
        traceback.print_exc()
    finally:
        ctx.pop()




def process_device_status(device_id, data):
    """Process device status updates"""
    ctx = app.app_context()
    ctx.push()
    try:
        status = data.get('status', 'online')
        
        device = Device.query.get(device_id)
        if device:
            device.status = status
            device.last_seen = datetime.now(timezone.utc)
            db.session.commit()
            
            # Notify frontend
            socketio.emit('device_update', {
                'device_id': device_id,
                'status': status,
                'last_seen': device.last_seen.isoformat()
            })
    except Exception as e:
        db.session.rollback()
        print(f"Error updating device status: {e}")
    finally:
        ctx.pop()


def broadcast_parking_update():
    """Broadcast parking updates to all connected WebSocket clients"""
    try:
        parking_data = get_all_parking_data()
        socketio.emit('parking_update', parking_data)
    except Exception as e:
        print(f"Error broadcasting update: {e}")


def init_mqtt():
    """Initialize MQTT client"""
    global mqtt_client
    print(f"\nInitializing MQTT client...")
    print(f"  Broker: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"  Username: {MQTT_USER}")
    
    # Use unique client ID based on process ID to avoid conflicts
    import os as os_module
    client_id = f"flask_backend_{os_module.getpid()}"
    
    mqtt_client = mqtt.Client(client_id=client_id, protocol=mqtt.MQTTv311, clean_session=True)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    
    # Add disconnect callback for debugging
    def on_disconnect(client, userdata, rc):
        if rc != 0:
            print(f"Unexpected MQTT disconnect: {rc}")
    mqtt_client.on_disconnect = on_disconnect
    
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
    except Exception as e:
        print(f"Failed to connect to MQTT broker: {e}")
        import traceback
        traceback.print_exc()


# REST API Endpoints

@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'mqtt_connected': mqtt_client.is_connected() if mqtt_client else False
    })


@app.route('/api/debug/devices', methods=['GET'])
def debug_devices():
    """Debug endpoint to see all registered devices and their sensors"""
    devices = Device.query.all()
    result = []
    for device in devices:
        sensors = DistanceSensor.query.filter_by(device_id=device.id).all()
        result.append({
            'id': device.id,
            'name': device.name,
            'mac_address': device.mac_address,
            'status': device.status,
            'mqtt_username': f"esp32_{device.mac_address.replace(':', '').upper()}",
            'sensors_count': len(sensors),
            'sensors': [
                {
                    'id': s.id,
                    'index': s.index,
                    'name': s.name,
                    'distance': s.current_distance,
                    'occupied': s.is_occupied
                } for s in sensors
            ]
        })
    return jsonify({
        'total_devices': len(devices),
        'devices': result
    })


# Device Registration Endpoints

@app.route('/api/device/register', methods=['POST'])
def register_iot_device():
    """
    ESP32 Device Registration Endpoint - Simplified Flow
    
    ESP32 registers via HTTP and receives MQTT credentials.
    Sensors are auto-registered when they send data via MQTT.
    
    Expected JSON from ESP32:
    {
        "mac_address": "AA:BB:CC:DD:EE:FF",
        "name": "ESP32 Parking Sensor",
        "location": "Parking Lot A - Spot 1",
        "latitude": 46.7712,
        "longitude": 23.6236
    }
    
    Returns:
    {
        "device_id": 123,
        "mqtt_username": "esp32_AABBCCDDEEFF",
        "mqtt_password": "esp32_pass_AABBCCDDEEFF",
        "mqtt_broker": "192.168.1.103",
        "mqtt_port": 1883,
        "sensor_topic": "device/123/sensors",
        "status": "registered"
    }
    """
    try:
        data = request.get_json()
        
        # Validate required fields
        required_fields = ['mac_address', 'name', 'location']
        for field in required_fields:
            if not data.get(field):
                return jsonify({'error': f'{field} is required'}), 400
        
        mac_address_orig = data.get('mac_address')
        mac_address_clean = mac_address_orig.replace(':', '').upper()
        name = data.get('name')
        location = data.get('location')
        latitude = data.get('latitude', 0.0)
        longitude = data.get('longitude', 0.0)
        
        # Check if device already exists
        existing_device = Device.query.filter_by(mac_address=mac_address_orig).first()
        if existing_device:
            # Return existing device info
            mqtt_username = f"esp32_{mac_address_clean}"
            mqtt_password = f"esp32_pass_{mac_address_clean}"
            
            # Ensure MQTT credentials exist even for already-registered devices
            create_mqtt_user(mqtt_username, mqtt_password)
            
            return jsonify({
                'device_id': existing_device.id,
                'mqtt_username': mqtt_username,
                'mqtt_password': mqtt_password,
                'mqtt_broker': MQTT_BROKER,
                'mqtt_port': MQTT_PORT,
                'sensor_topic': f'device/{existing_device.id}/sensors',
                'status': existing_device.status,
                'message': 'Device already registered'
            }), 200
        
        # Create new device
        new_device = Device(
            mac_address=mac_address_orig,
            name=name,
            location=location,
            latitude=latitude,
            longitude=longitude,
            status='registered'
        )
        
        db.session.add(new_device)
        db.session.commit()
        
        # Generate MQTT credentials matching ESP32 format
        mqtt_username = f"esp32_{mac_address_clean}"
        mqtt_password = f"esp32_pass_{mac_address_clean}"
        
        # Create MQTT user in mosquitto
        create_mqtt_user(mqtt_username, mqtt_password)
        
        # Notify frontend about new device
        socketio.emit('device_registered', {
            'device_id': new_device.id,
            'name': name,
            'location': location,
            'latitude': float(latitude) if latitude else 0.0,
            'longitude': float(longitude) if longitude else 0.0,
            'status': 'registered'
        })
        
        return jsonify({
            'device_id': new_device.id,
            'mqtt_username': mqtt_username,
            'mqtt_password': mqtt_password,
            'mqtt_broker': MQTT_BROKER,
            'mqtt_port': MQTT_PORT,
            'sensor_topic': f'device/{new_device.id}/sensors',
            'status': 'registered',
            'message': 'ESP32 device registered successfully'
        }), 201
        
    except Exception as e:
        db.session.rollback()
        print(f"Error registering ESP32 device: {e}")
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
            total_spots = len(sensors)
            available_spots = sum(1 for s in sensors if not s.is_occupied)
            
            # Determine actual device status based on last_seen timestamp
            actual_status = 'online' if is_device_online(device) else 'offline'
            
            # Update device status in database if it changed
            if device.status != actual_status and actual_status == 'offline':
                device.status = 'offline'
                db.session.commit()
            
            device_data = {
                'id': device.id,
                'name': device.name,
                'location': device.location,
                'latitude': float(device.latitude) if device.latitude else None,
                'longitude': float(device.longitude) if device.longitude else None,
                'status': actual_status,
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
        
        # Determine actual device status based on last_seen timestamp
        actual_status = 'online' if is_device_online(device) else 'offline'
        
        # Update device status in database if it changed
        if device.status != actual_status and actual_status == 'offline':
            device.status = 'offline'
            db.session.commit()
        
        device_data = {
            'id': device.id,
            'name': device.name,
            'location': device.location,
            'latitude': float(device.latitude) if device.latitude else None,
            'longitude': float(device.longitude) if device.longitude else None,
            'status': actual_status,
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


def check_device_timeouts():
    """Background task to periodically check for offline devices and notify frontend"""
    import time
    while True:
        try:
            time.sleep(DEVICE_TIMEOUT // 2)  # Check twice per timeout period
            
            with app.app_context():
                devices = Device.query.filter_by(status='online').all()
                devices_went_offline = []
                
                for device in devices:
                    if not is_device_online(device):
                        device.status = 'offline'
                        devices_went_offline.append(device.id)
                
                if devices_went_offline:
                    db.session.commit()
                    # Broadcast updated parking data to all clients
                    parking_data = get_all_parking_data()
                    socketio.emit('parking_update', parking_data)
                    
        except Exception as e:
            print(f"Error in device timeout checker: {e}")


# WebSocket Events

@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    # Send current parking status to newly connected client
    parking_data = get_all_parking_data()
    emit('parking_update', parking_data)


@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    pass


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
        
        # Populate database with dummy data if empty
        if Device.query.count() == 0:
            from data.add_dummy_data import add_dummy_data
            print("Database is empty. Adding dummy data...")
            add_dummy_data()
    
    # Initialize MQTT client
    init_mqtt()
    
    # Start background device timeout checker
    timeout_thread = threading.Thread(target=check_device_timeouts, daemon=True)
    timeout_thread.start()
    print(f"Device timeout checker started (timeout: {DEVICE_TIMEOUT}s)")
    
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
