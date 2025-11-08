FROM python:3.11-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

# Create data directory and ensure permissions
RUN mkdir -p /app/data && chmod 755 /app/data

EXPOSE 5000
CMD ["python", "app.py"]