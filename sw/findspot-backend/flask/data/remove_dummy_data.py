#!/usr/bin/env python3
"""
Remove dummy data from FindSpot database, keeping only real ESP32 devices
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

def remove_dummy_data():
    """Remove dummy data while keeping real ESP32 devices"""
    with app.app_context():
        print("Removing dummy data...")
        
        # List of dummy MAC addresses from add_dummy_data.py
        dummy_macs = [
            '00:11:22:33:44:55',
            '00:11:22:33:44:56',
            '00:11:22:33:44:57',
            '00:11:22:33:44:58',
            '00:11:22:33:44:59',
            '00:11:22:33:44:60'
        ]
        
        # Find and delete dummy devices
        dummy_devices = Device.query.filter(Device.mac_address.in_(dummy_macs)).all()
        
        if not dummy_devices:
            print("No dummy data found in database")
            return
        
        print(f"Found {len(dummy_devices)} dummy devices:")
        for device in dummy_devices:
            sensors_count = len(device.distance_sensors)
            cameras_count = len(device.cameras)
            print(f"  - {device.name} (ID: {device.id}, MAC: {device.mac_address})")
            print(f"    {sensors_count} sensors, {cameras_count} cameras")
        
        # Delete dummy devices (cascade will delete associated sensors and cameras)
        for device in dummy_devices:
            db.session.delete(device)
        
        db.session.commit()
        print(f"Removed {len(dummy_devices)} dummy devices and their associated sensors/cameras")
        
        # Show remaining devices
        remaining_devices = Device.query.all()
        print(f"\nRemaining devices: {len(remaining_devices)}")
        for device in remaining_devices:
            sensors_count = len(device.distance_sensors)
            print(f"  - {device.name} (ID: {device.id}, MAC: {device.mac_address}, Status: {device.status})")
            print(f"    {sensors_count} sensors")

if __name__ == '__main__':
    remove_dummy_data()
    print("\nDone!")
