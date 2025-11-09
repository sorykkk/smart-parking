FROM python:3.11-slim

WORKDIR /app

# Install mosquitto (provides mosquitto_passwd) and mosquitto-clients for pub/sub testing
RUN apt-get update && \
    apt-get install -y --no-install-recommends mosquitto mosquitto-clients && \
    rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

# Ensure data directory exists with full permissions
RUN mkdir -p /app/data && chmod -R 777 /app/data

EXPOSE 5000
CMD ["python", "app.py"]