#!/usr/bin/env python3
"""
ESP32 Smart Parking Mock Simulator
Simulates ESP32 device behavior without physical hardware
Connects to backend, registers device, and publishes dummy sensor data via MQTT
"""

import paho.mqtt.client as mqtt
import requests
import json
import time
import random
import hashlib
from datetime import datetime

# ==================== Configuration ============================ //
# Backend Configuration
BACKEND_HOST = "localhost"  # Change to your backend IP if needed
BACKEND_PORT = 5000

# WiFi Simulation (not actually used, just for display)
WIFI_SSID = "Cam405-17"

# Device Configuration (matching Arduino code)
DEVICE_PREFIX = "esp32_dev"
DEVICE_LOCATION = "Complexul Studentesc P1"
DEVICE_LATITUDE = 45.74956539097931
DEVICE_LONGITUDE = 21.240075184660427

# Sensor Configuration
DISTANCE_MIN_CM = 5
DISTANCE_MAX_CM = 50  # Below this = occupied
SENSOR_READ_INTERVAL = 1.0  # seconds (matching Arduino 1000ms)

# Number of sensors to simulate
NUM_SENSORS = 3


# ==================== Dummy Sensor Class ============================ //
class DummySensor:
    """Mock sensor that simulates distance readings"""
    
    def __init__(self, device_id, technology, index):
        self.device_id = device_id
        self.technology = technology
        self.index = index
        self.current_distance = 100.0
        self.occupied = False
        self.cycle_counter = 0
    
    def check_state(self):
        """Simulate distance reading with cycling patterns"""
        self.cycle_counter += 1
        
        # Different patterns for each sensor (matching Arduino logic)
        if self.index == 0:
            # Sensor 0: Changes every 3 readings (fast)
            if (self.cycle_counter // 3) % 2 == 0:
                self.current_distance = 60.0  # Free
                self.occupied = False
            else:
                self.current_distance = 25.0  # Occupied
                self.occupied = True
                
        elif self.index == 1:
            # Sensor 1: Changes every 5 readings (medium)
            if (self.cycle_counter // 5) % 2 == 0:
                self.current_distance = 70.0  # Free
                self.occupied = False
            else:
                self.current_distance = 15.0  # Occupied
                self.occupied = True
                
        elif self.index == 2:
            # Sensor 2: Random-like pattern using modulo
            if (self.cycle_counter % 7) < 4:
                self.current_distance = 80.0  # Free
                self.occupied = False
            else:
                self.current_distance = 30.0  # Occupied
                self.occupied = True
        else:
            # Additional sensors: simple alternation
            if (self.cycle_counter // 2) % 2 == 0:
                self.current_distance = 65.0  # Free
                self.occupied = False
            else:
                self.current_distance = 20.0  # Occupied
                self.occupied = True
        
        return self.occupied
    
    def to_json(self):
        """Generate JSON payload matching ESP32 format"""
        return {
            "device_id": self.device_id,
            "sensor_index": self.index,
            "technology": self.technology,
            "occupied": self.occupied,
            "distance_cm": self.current_distance,
            "timestamp": int(time.time() * 1000),
            # Additional fields to match real sensor
            "name": f"sensor_{self.index}",
            "type": "distance",
            "current_distance": self.current_distance,
            "is_occupied": self.occupied,
            "trigger_pin": 13 + self.index * 2,
            "echo_pin": 12 + self.index * 2
        }


# ==================== ESP32 Mock Simulator ============================ //
class ESP32Simulator:
    def __init__(self):
        self.device_id = -1
        self.mqtt_username = None
        self.mqtt_password = None
        self.mqtt_broker = None
        self.mqtt_port = 1883
        self.sensor_topic = None
        self.mqtt_client = None
        self.sensors = []
        self.sensor_states = []
        self.mac_address = self._generate_mac_address()
        self.device_name = f"{DEVICE_PREFIX}_{self.mac_address[-8:]}"
        
    def _generate_mac_address(self):
        """Generate a fake but consistent MAC address"""
        # Use a hash of hostname + timestamp for uniqueness
        import socket
        seed = f"{socket.gethostname()}_{int(time.time())}"
        hash_val = hashlib.md5(seed.encode()).hexdigest()[:12]
        return ":".join([hash_val[i:i+2] for i in range(0, 12, 2)])
    
    def print_header(self, text):
        """Print formatted header"""
        print("\n" + "="*60)
        print(f"  {text}")
        print("="*60)
    
    def register_device(self):
        """Register device with backend via HTTP"""
        self.print_header("DEVICE REGISTRATION")
        
        url = f"http://{BACKEND_HOST}:{BACKEND_PORT}/api/device/register"
        
        payload = {
            "mac_address": self.mac_address,
            "name": self.device_name,
            "location": DEVICE_LOCATION,
            "latitude": DEVICE_LATITUDE,
            "longitude": DEVICE_LONGITUDE
        }
        
        print(f"Registering device with backend...")
        print(f"URL: {url}")
        print(f"Payload: {json.dumps(payload, indent=2)}")
        
        try:
            response = requests.post(url, json=payload, timeout=10)
            print(f"\nHTTP Response Code: {response.status_code}")
            print(f"Response: {response.text}")
            
            if response.status_code in [200, 201]:
                data = response.json()
                self.device_id = data['device_id']
                self.mqtt_username = data['mqtt_username']
                self.mqtt_password = data['mqtt_password']
                self.mqtt_broker = data['mqtt_broker']
                self.mqtt_port = data['mqtt_port']
                self.sensor_topic = data['sensor_topic']
                
                print(f"\n✓ Registration successful!")
                print(f"  Device ID: {self.device_id}")
                print(f"  MQTT Username: {self.mqtt_username}")
                print(f"  MQTT Broker: {self.mqtt_broker}:{self.mqtt_port}")
                print(f"  Sensor Topic: {self.sensor_topic}")
                return True
            else:
                print(f"\n✗ Registration failed: {response.text}")
                return False
                
        except requests.exceptions.RequestException as e:
            print(f"\n✗ HTTP Error: {e}")
            return False
    
    def mqtt_on_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            print(f"\n✓ Connected to MQTT Broker at {self.mqtt_broker}:{self.mqtt_port}")
            print(f"  Client ID: {DEVICE_PREFIX}{self.device_id}")
            print(f"  Username: {self.mqtt_username}")
        else:
            print(f"\n✗ MQTT Connection failed with code {rc}")
    
    def mqtt_on_disconnect(self, client, userdata, rc):
        """MQTT disconnect callback"""
        if rc != 0:
            print(f"\n⚠ Unexpected MQTT disconnect (code {rc}). Reconnecting...")
    
    def mqtt_on_message(self, client, userdata, msg):
        """MQTT message callback"""
        print(f"\n📨 MQTT Message on topic: {msg.topic}")
        print(f"📦 Payload: {msg.payload.decode()}")
    
    def connect_mqtt(self):
        """Connect to MQTT broker"""
        self.print_header("MQTT CONNECTION")
        
        client_id = f"{DEVICE_PREFIX}{self.device_id}"
        
        self.mqtt_client = mqtt.Client(client_id=client_id)
        self.mqtt_client.username_pw_set(self.mqtt_username, self.mqtt_password)
        self.mqtt_client.on_connect = self.mqtt_on_connect
        self.mqtt_client.on_disconnect = self.mqtt_on_disconnect
        self.mqtt_client.on_message = self.mqtt_on_message
        
        print(f"Connecting to MQTT broker...")
        print(f"  Broker: {self.mqtt_broker}:{self.mqtt_port}")
        print(f"  Client ID: {client_id}")
        print(f"  Username: {self.mqtt_username}")
        
        try:
            self.mqtt_client.connect(self.mqtt_broker, self.mqtt_port, 60)
            self.mqtt_client.loop_start()
            
            # Wait for connection
            time.sleep(2)
            
            if self.mqtt_client.is_connected():
                print("✓ MQTT Connected successfully")
                return True
            else:
                print("✗ MQTT Connection timeout")
                return False
                
        except Exception as e:
            print(f"✗ MQTT Connection error: {e}")
            return False
    
    def initialize_sensors(self):
        """Initialize dummy sensors"""
        self.print_header("SENSOR INITIALIZATION")
        
        print(f"Initializing {NUM_SENSORS} dummy sensors...")
        
        for i in range(NUM_SENSORS):
            sensor = DummySensor(self.device_id, "ultrasonic_dummy", i)
            self.sensors.append(sensor)
            self.sensor_states.append(False)
            print(f"  Sensor {i}: Initialized")
        
        print(f"\n✓ Initialized {len(self.sensors)} sensors")
        print("\nDummy Sensor Patterns:")
        print("  Sensor 0: Changes every 3 readings (fast)")
        print("  Sensor 1: Changes every 5 readings (medium)")
        print("  Sensor 2: Changes in 4/7 pattern (irregular)")
    
    def publish_sensor_data(self, sensor_index, payload):
        """Publish sensor data to MQTT"""
        topic = f"device/{self.device_id}/sensors/{sensor_index}"
        payload_str = json.dumps(payload)
        
        try:
            result = self.mqtt_client.publish(topic, payload_str, qos=1)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                return True
            else:
                print(f"    ✗ MQTT Publish failed with code {result.rc}")
                return False
        except Exception as e:
            print(f"    ✗ MQTT Publish error: {e}")
            return False
    
    def run_sensor_loop(self):
        """Main sensor reading loop"""
        self.print_header("STARTING SENSOR LOOP")
        
        print("Monitoring sensors for state changes...")
        print("Press Ctrl+C to stop\n")
        
        try:
            while True:
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                print(f"\n[{timestamp}] ========== Reading Sensors ==========")
                
                for i, sensor in enumerate(self.sensors):
                    current_state = sensor.check_state()
                    
                    print(f"  Sensor {i}: ", end="")
                    print(f"{'OCCUPIED' if current_state else 'FREE'} ", end="")
                    print(f"(was: {'OCCUPIED' if self.sensor_states[i] else 'FREE'})", end="")
                    print(f" - Distance: {sensor.current_distance:.1f} cm")
                    
                    # Publish if state changed
                    if current_state != self.sensor_states[i]:
                        print(f"    ↳ State changed! Publishing...", end="")
                        
                        payload = sensor.to_json()
                        
                        if self.publish_sensor_data(i, payload):
                            self.sensor_states[i] = current_state
                            print(" ✓ Published")
                            print(f"    ↳ Payload: {json.dumps(payload, indent=6)}")
                        else:
                            print(" ✗ Failed")
                
                print("=" * 60)
                
                # Wait for next reading
                time.sleep(SENSOR_READ_INTERVAL)
                
        except KeyboardInterrupt:
            print("\n\n⚠ Interrupted by user")
            self.cleanup()
    
    def cleanup(self):
        """Cleanup on exit"""
        print("\nCleaning up...")
        if self.mqtt_client:
            self.mqtt_client.loop_stop()
            self.mqtt_client.disconnect()
        print("✓ Disconnected from MQTT")
        print("Goodbye!")
    
    def run(self):
        """Main execution flow"""
        print("\n")
        print("╔═══════════════════════════════════════════════════════════╗")
        print("║   ESP32 Smart Parking Mock Simulator (Python)             ║")
        print("║   No Hardware Required - Full Backend Integration         ║")
        print("╚═══════════════════════════════════════════════════════════╝")
        
        print(f"\nConfiguration:")
        print(f"  Backend: http://{BACKEND_HOST}:{BACKEND_PORT}")
        print(f"  Device Name: {self.device_name}")
        print(f"  MAC Address: {self.mac_address}")
        print(f"  Location: {DEVICE_LOCATION}")
        print(f"  Sensors: {NUM_SENSORS}")
        print(f"  Update Interval: {SENSOR_READ_INTERVAL}s")
        
        # Step 1: Register device
        if not self.register_device():
            print("\n✗ Failed to register device. Exiting.")
            return
        
        # Step 2: Connect to MQTT
        if not self.connect_mqtt():
            print("\n✗ Failed to connect to MQTT. Exiting.")
            return
        
        # Step 3: Initialize sensors
        self.initialize_sensors()
        
        # Step 4: Run sensor loop
        self.run_sensor_loop()


# ==================== Main Entry Point ============================ //
if __name__ == "__main__":
    simulator = ESP32Simulator()
    simulator.run()
