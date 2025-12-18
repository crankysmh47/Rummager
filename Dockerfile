# Use a multi-stage build to reduce image size

# --- Stage 1: Build Frontend ---
FROM node:18-alpine AS frontend-builder
WORKDIR /app/frontend
# Copy package files first for better caching
COPY frontend/package*.json ./
RUN npm ci
# Copy source code
COPY frontend/ ./
# Build React App
RUN npm run build

# --- Stage 2: Build Backend & Final Image ---
FROM python:3.9-slim

# Install system dependencies for C++ compilation
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy C++ Source
COPY *.cpp *.h ./

# Compile C++ Engine (Optimized)
RUN g++ -O3 -std=c++17 searchengine.cpp -o searchengine
RUN g++ -O3 -std=c++17 trie_builder.cpp -o trie_builder

# Install Python Requirements
COPY requirements.txt .
# If requirements.txt doesn't exist, install manually
RUN pip install --no-cache-dir flask flask-cors requests uvicorn

# Copy Backend Scripts
COPY main.py .
# Copy Data Files (Ensure .dockerignore excludes heavy ones if desired, but we need them for search)
# CAUTION: If data is 2.9M files, this COPY will be huge. 
# Best practice: Mount data as a volume. But for "one-click" demo, we might copy specific bins.
# We will copy everything not ignored by .dockerignore (which should exclude venv, etc)
COPY *.bin ./
COPY doc_metadata.txt ./
COPY barrels/ ./barrels/

# Copy Frontend Build from Stage 1 to 'static'
COPY --from=frontend-builder /app/frontend/dist ./static

# Expose Port
EXPOSE 8000

# Run
CMD ["python", "main.py"]
