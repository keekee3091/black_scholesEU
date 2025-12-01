FROM ubuntu:22.04

RUN dpkg --add-architecture i386 && \
    apt-get update && apt-get install -y \
    python3 python3-pip \
    wine64 wine32 winbind \
    && apt-get clean

WORKDIR /app

COPY ./fetch_data.py ./fetch_data.py
COPY ./cache ./cache

RUN pip3 install fastapi uvicorn yfinance pandas requests openpyxl

COPY ./x64/Release/black_scholes_algo.exe ./api_cpp.exe
COPY ./x64/Release/libcurl.dll ./libcurl.dll
COPY ./x64/Release/zlib1.dll ./zlib1.dll
COPY ./x64/Release/cpr.dll ./cpr.dll

COPY ./docker/run.sh ./run.sh
RUN chmod +x run.sh

EXPOSE 8000 8080

CMD ["./run.sh"]
