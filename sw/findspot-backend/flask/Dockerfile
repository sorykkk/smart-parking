FROM python:3.11-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

# Ensure data directory exists with full permissions
RUN mkdir -p /app/data && chmod -R 777 /app/data

EXPOSE 5000
CMD ["python", "app.py"]