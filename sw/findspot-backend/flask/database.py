"""
Database initialization and configuration
"""

from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()


def init_db():
    """Initialize database and create tables"""
    db.create_all()
    print("✓ Database initialized")

