"""
Database models for FindSpot parking system
"""

from database import db
from datetime import datetime, timezone
from sqlalchemy import Numeric

# Devices represents cluster/individual places
# So there is no need to do separate table for parking spots

class Device(db.Model):
    """Represents an ESP32 device or any other which supports wifi connection"""
    __tablename__ = 'device'
    
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    location = db.Column(db.String(150))
    latitude = db.Column(Numeric(precision=14, scale=7), nullable=False)
    longitude = db.Column(Numeric(precision=14, scale=7), nullable=False)
    mac_address = db.Column(db.String(17), unique=True, nullable=False)
    status = db.Column(db.String(50), default='registered')  # registered, online, offline, error
    last_seen = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    registered_at = db.Column(db.DateTime, default = lambda: datetime.now(timezone.utc))
    # Relationships  
    distance_sensors = db.relationship('DistanceSensor', backref='device', lazy=True, cascade='all, delete-orphan')
    cameras = db.relationship('Camera', backref='device', lazy=True, cascade='all, delete-orphan')
    
    def __repr__(self):
        return f'<Device {self.id}>'

class DistanceSensor(db.Model):
    """Represents a distance sensor attached to a device"""
    __tablename__ = 'distance_sensors'
    
    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.Integer, db.ForeignKey('device.id'), nullable=False)
    index = db.Column(db.Integer, nullable=False)
    name = db.Column(db.String(100), nullable=False)  # e.g., "ultrasonic_0_esp32_dev_1"
    type = db.Column(db.String(50), default='distance')  
    technology = db.Column(db.String(50), default='ultrasonic') # ultrasonic, infrared, etc.
    trigger_pin = db.Column(db.Integer)
    echo_pin = db.Column(db.Integer)
    current_distance = db.Column(db.BigInteger)  # in cm
    is_occupied = db.Column(db.Boolean, default=False)
    last_updated = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    
    # Unique constraint: each device can have only one sensor with a specific index
    __table_args__ = (db.UniqueConstraint('device_id', 'index', name='_device_sensor_index_uc'),)
    
    def __repr__(self):
        return f'<DistanceSensor {self.name} on Device {self.device_id}>'


class Camera(db.Model):
    """Represents a camera attached to a device"""
    __tablename__ = 'cameras'
    
    id = db.Column(db.Integer, primary_key=True)
    device_id = db.Column(db.Integer, db.ForeignKey('device.id'), nullable=False)
    index = db.Column(db.Integer, nullable=False)
    type = db.Column(db.String(50), default='camera')
    technology = db.Column(db.String(50), default="OV2640")
    name = db.Column(db.String(100), nullable=False)  # e.g., "esp-cam-1"
    resolution = db.Column(db.String(50))  # e.g., "QVGA", "VGA"
    jpeg_quality = db.Column(db.Integer)  # JPEG quality setting
    image_size = db.Column(db.Integer)
    image_base64 = db.Column(db.Text)  # Use Text for large base64 strings
    last_updated = db.Column(db.DateTime)
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    # fields completed in the backend
    plate_registered = db.Column(db.Boolean, default=False)
    plate_number = db.Column(db.String(10))
    # Unique constraint: each device can have only one camera with a specific index
    __table_args__ = (db.UniqueConstraint('device_id', 'index', name='_device_camera_index_uc'),)
    
    def __repr__(self):
        return f'<Camera {self.name} on Device {self.device_id}>'