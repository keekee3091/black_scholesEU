#!/bin/bash

echo "Starting FastAPI..."
uvicorn fetch_data:app --host 0.0.0.0 --port 8000 &

sleep 2

echo "Starting C++ engine..."
./Project1/build/api_cpp
