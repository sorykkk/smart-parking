FROM python:3.14-slim

WORKDIR /app

# Install mosquitto-clients for mosquitto_passwd command
RUN apt-get update && \
    apt-get install -y --no-install-recommends mosquitto-clients && \
    rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

# Ensure data directory exists with full permissions
RUN mkdir -p /app/data && chmod -R 777 /app/data

EXPOSE 5000
CMD ["python", "app.py"]