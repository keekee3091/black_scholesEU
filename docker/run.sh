#!/bin/bash

echo "Starting FastAPI..."
uvicorn fetch_data:app --host 0.0.0.0 --port 8000 &

sleep 2

echo "Starting C++ Windows EXE with Wine..."
wine64 api_cpp.exe
