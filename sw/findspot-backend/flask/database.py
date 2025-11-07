"""
Database initialization and configuration
"""

from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()


def init_db():
    """Initialize database and create tables"""
    db.create_all()
    
    # Create default parking location if none exists
    from models import ParkingLocation
    
    if ParkingLocation.query.count() == 0:
        default_location = ParkingLocation(
            name="Default Parking Area",
            latitude=45.7489,  # Example: Cluj-Napoca, Romania
            longitude=21.2087,
            address="Example Street, Cluj-Napoca"
        )
        db.session.add(default_location)
        db.session.commit()
        print("✓ Created default parking location")
    
    print("✓ Database initialized")
