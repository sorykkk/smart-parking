#!/bin/bash
# Script to generate self-signed SSL certificates for development
# For production, use Let's Encrypt with certbot

mkdir -p ssl

openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout ssl/key.pem \
  -out ssl/cert.pem \
  -subj "/C=RO/ST=Timis/L=Timisoara/O=FindSpot/CN=findspot.local"

echo "Self-signed certificates generated in ssl/ directory"
echo "For production, replace with Let's Encrypt certificates"
