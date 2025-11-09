FROM python:3.11-slim

WORKDIR /app

# Install mosquitto (provides mosquitto_passwd), mosquitto-clients, and Docker CLI for container management
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        mosquitto \
        mosquitto-clients \
        curl \
        ca-certificates && \
    install -m 0755 -d /etc/apt/keyrings && \
    curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc && \
    chmod a+r /etc/apt/keyrings/docker.asc && \
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/debian bookworm stable" > /etc/apt/sources.list.d/docker.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends docker-ce-cli && \
    rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

# Ensure data directory exists with full permissions
RUN mkdir -p /app/data && chmod -R 777 /app/data

EXPOSE 5000
CMD ["python", "app.py"]