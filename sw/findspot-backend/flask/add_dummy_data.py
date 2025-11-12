#!/usr/bin/env python3
"""
Add dummy data to FindSpot database for testing
"""

import os
import sys
from datetime import datetime, timezone
from dotenv import load_dotenv

# Add the current directory to the Python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Load environment variables
parent_env = os.path.join(os.path.dirname(os.path.dirname(__file__)), '.env')
current_env = '.env'

if os.path.exists(parent_env):
    load_dotenv(parent_env)
    print(f"Loaded environment from: {parent_env}")
elif os.path.exists(current_env):
    load_dotenv(current_env)
    print(f"Loaded environment from: {current_env}")

from app import app
from database import db
from models import Device, DistanceSensor, Camera

def add_dummy_data():
    """Add dummy parking data for testing"""
    with app.app_context():
        # Clear existing data
        print("Clearing existing data...")
        DistanceSensor.query.delete()
        Camera.query.delete()
        Device.query.delete()
        db.session.commit()
        
        # Cluj-Napoca coordinates for reference: 46.7712, 23.6236
        
        # Create dummy devices (parking locations)
        devices_data = [
            {
                'name': 'Central Park Parking',
                'location': 'Piața Unirii 1, Cluj-Napoca',
                'latitude': 46.7712,
                'longitude': 23.6236,
                'mac_address': '00:11:22:33:44:55',
                'status': 'online',
                'spots': [
                    {'name': 'Spot A1', 'index': 0, 'occupied': False, 'distance': 25},
                    {'name': 'Spot A2', 'index': 1, 'occupied': True, 'distance': 8},
                    {'name': 'Spot A3', 'index': 2, 'occupied': False, 'distance': 30},
                    {'name': 'Spot A4', 'index': 3, 'occupied': False, 'distance': 28},
                ]
            },
            {
                'name': 'University Area Parking',
                'location': 'Str. Mihail Kogălniceanu 1, Cluj-Napoca',
                'latitude': 46.7693,
                'longitude': 23.5893,
                'mac_address': '00:11:22:33:44:56',
                'status': 'online',
                'spots': [
                    {'name': 'Spot B1', 'index': 0, 'occupied': True, 'distance': 7},
                    {'name': 'Spot B2', 'index': 1, 'occupied': True, 'distance': 6},
                    {'name': 'Spot B3', 'index': 2, 'occupied': False, 'distance': 35},
                ]
            },
            {
                'name': 'Mall Parking',
                'location': 'Strada Fabricii 1, Cluj-Napoca',
                'latitude': 46.7733,
                'longitude': 23.6203,
                'mac_address': '00:11:22:33:44:57',
                'status': 'online',
                'spots': [
                    {'name': 'Spot C1', 'index': 0, 'occupied': False, 'distance': 40},
                    {'name': 'Spot C2', 'index': 1, 'occupied': False, 'distance': 38},
                    {'name': 'Spot C3', 'index': 2, 'occupied': False, 'distance': 42},
                    {'name': 'Spot C4', 'index': 3, 'occupied': False, 'distance': 39},
                    {'name': 'Spot C5', 'index': 4, 'occupied': True, 'distance': 5},
                    {'name': 'Spot C6', 'index': 5, 'occupied': False, 'distance': 45},
                ]
            },
            {
                'name': 'Train Station Parking',
                'location': 'Piața Gării 1, Cluj-Napoca',
                'latitude': 46.7584,
                'longitude': 23.5960,
                'mac_address': '00:11:22:33:44:58',
                'status': 'online',
                'spots': [
                    {'name': 'Spot D1', 'index': 0, 'occupied': True, 'distance': 9},
                    {'name': 'Spot D2', 'index': 1, 'occupied': True, 'distance': 8},
                    {'name': 'Spot D3', 'index': 2, 'occupied': True, 'distance': 7},
                    {'name': 'Spot D4', 'index': 3, 'occupied': True, 'distance': 6},
                ]
            },
            {
                'name': 'Business District Parking',
                'location': 'Strada Dorobantilor 50, Cluj-Napoca',
                'latitude': 46.7799,
                'longitude': 23.6126,
                'mac_address': '00:11:22:33:44:59',
                'status': 'offline',  # Offline device for testing
                'spots': [
                    {'name': 'Spot E1', 'index': 0, 'occupied': False, 'distance': 50},
                    {'name': 'Spot E2', 'index': 1, 'occupied': False, 'distance': 48},
                ]
            },
            {
                'name': 'Hospital Parking',
                'location': 'Strada Clinicilor 3-5, Cluj-Napoca',
                'latitude': 46.7587,
                'longitude': 23.6410,
                'mac_address': '00:11:22:33:44:60',
                'status': 'online',
                'spots': [
                    {'name': 'Spot F1', 'index': 0, 'occupied': False, 'distance': 32},
                    {'name': 'Spot F2', 'index': 1, 'occupied': False, 'distance': 29},
                    {'name': 'Spot F3', 'index': 2, 'occupied': True, 'distance': 4},
                ]
            }
        ]
        
        print("Adding dummy devices and sensors...")
        
        for device_data in devices_data:
            # Create device
            device = Device(
                name=device_data['name'],
                location=device_data['location'],
                latitude=device_data['latitude'],
                longitude=device_data['longitude'],
                mac_address=device_data['mac_address'],
                status=device_data['status'],
                last_seen=datetime.now(timezone.utc),
                created_at=datetime.now(timezone.utc),
                registered_at=datetime.now(timezone.utc)
            )
            
            db.session.add(device)
            db.session.flush()  # Get the device ID
            
            # Create distance sensors for this device
            for spot_data in device_data['spots']:
                sensor = DistanceSensor(
                    device_id=device.id,
                    index=spot_data['index'],
                    name=spot_data['name'],
                    type='distance',
                    technology='ultrasonic',
                    trigger_pin=2,
                    echo_pin=4,
                    current_distance=spot_data['distance'],
                    is_occupied=spot_data['occupied'],
                    last_updated=datetime.now(timezone.utc),
                    created_at=datetime.now(timezone.utc)
                )
                db.session.add(sensor)
            
            print(f"Added device: {device_data['name']} with {len(device_data['spots'])} parking spots")
        
        db.session.commit()
        print("\n✅ Dummy data added successfully!")
        
        # Print summary
        total_devices = Device.query.count()
        total_sensors = DistanceSensor.query.count()
        available_spots = DistanceSensor.query.filter_by(is_occupied=False).count()
        occupied_spots = DistanceSensor.query.filter_by(is_occupied=True).count()
        
        print(f"\n📊 Database Summary:")
        print(f"   Devices: {total_devices}")
        print(f"   Total Spots: {total_sensors}")
        print(f"   Available Spots: {available_spots}")
        print(f"   Occupied Spots: {occupied_spots}")
        
        print(f"\n🗺️  Test Locations in Cluj-Napoca:")
        for device in Device.query.all():
            available = DistanceSensor.query.filter_by(device_id=device.id, is_occupied=False).count()
            total = DistanceSensor.query.filter_by(device_id=device.id).count()
            print(f"   {device.name}: {available}/{total} available ({device.status})")

if __name__ == '__main__':
    try:
        add_dummy_data()
    except Exception as e:
        print(f"❌ Error adding dummy data: {e}")
        sys.exit(1)