FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    python3 python3-pip \
    python3-dev \
    g++ cmake git curl \
    libssl-dev \
    libboost-all-dev \
    && apt-get clean

WORKDIR /app

# Copie du code C++
COPY ./Project1 ./Project1

# Build C++
RUN mkdir -p /app/Project1/build && \
    cd /app/Project1/build && \
    cmake .. && \
    make -j8

# Backend Python
COPY ./fetch_data.py ./fetch_data.py
COPY ./cache ./cache

RUN pip3 install fastapi uvicorn yfinance pandas requests openpyxl

# Script
COPY ./docker/run.sh ./run.sh
RUN chmod +x run.sh

EXPOSE 8000 8080

CMD ["./run.sh"]
