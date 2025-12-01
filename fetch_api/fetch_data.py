import yfinance as yf
import pandas as pd
import time
import os
import asyncio
from concurrent.futures import ThreadPoolExecutor
from yfinance.exceptions import YFRateLimitError
import requests
from fastapi import FastAPI, Query, HTTPException
import uvicorn
from threading import Thread
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime

CACHE_DIR = "cache"
app = FastAPI()

@app.get('/ticker')
def get_ticker(symbol: str = Query(...)):
    symbol_list = [s.strip().upper() for s in symbol.split(",") if s.strip()]
    os.makedirs(CACHE_DIR, exist_ok=True)

    all_options = []

    def fetch_with_cache(sym):
        cache_file = os.path.join(CACHE_DIR, f"{sym}_ALL_options.xlsx")

        if os.path.exists(cache_file):
            print(f"[{sym}] Loading from cache")
            df = pd.read_excel(cache_file)
            return df.to_dict(orient="records")
        else:
            print(f"[{sym}] No cache. Loading...")
            options = fetch_options(sym)
            if not options:
                print(f"[{sym}] No option found")
                return []
            write_excel(options, filename=cache_file)
            return options

    with ThreadPoolExecutor(max_workers=8) as executor:
        futures = {executor.submit(fetch_with_cache, sym): sym for sym in symbol_list}

        for future in as_completed(futures):
            try:
                data = future.result()
                all_options.extend(data)
            except Exception as e:
                sym = futures[future]
                print(f"[{sym}] Error : {e}")

    if not all_options:
        raise HTTPException(status_code=404, detail="No option found for symbols")

    return all_options

@app.get('/historical')
def get_historical_prices(
    symbol: str = Query(...),
    from_date: str = Query(..., alias="from"),
    to_date: str = Query(..., alias="to")
):
    symbol_list = [s.strip().upper() for s in symbol.split(",") if s.strip()]
    results = {}

    try:
        start = datetime.strptime(from_date, "%Y-%m-%d").date()
        end = datetime.strptime(to_date, "%Y-%m-%d").date()
    except ValueError:
        return {"error": "Invalid date format. Use YYYY-MM-DD."}

    for sym in symbol_list:
        try:
            ticker = yf.Ticker(sym)
            hist = ticker.history(start=from_date, end=to_date)

            if hist.empty:
                results[sym] = {"error": "No historical data available"}
                continue

            hist = hist.reset_index()
            hist["Date"] = hist["Date"].dt.strftime("%Y-%m-%d")
            results[sym] = hist[["Date", "Open", "High", "Low", "Close"]].to_dict(orient="records")
            cache_file = os.path.join(CACHE_DIR, f"{sym}_Historical.xlsx")
            write_excel(results,cache_file)

        except Exception as e:
            results[sym] = {"error": str(e)}

    return results


def use_tor_proxy():
    print("[TOR] proxy socks5h://127.0.0.1:9050")

    class TorSession(requests.Session):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.proxies.update({
                'http': 'socks5h://127.0.0.1:9050',
                'https': 'socks5h://127.0.0.1:9050'
            })

    requests.Session = TorSession

def fetch_options_threaded(symbols, max_threads=4):
    all_options = []

    def safe_fetch(sym):
        try:
            return fetch_options(sym)
        except Exception as e:
            print(f"[{sym}] Error : {e}")
            return []

    with ThreadPoolExecutor(max_workers=max_threads) as executor:
        futures = {executor.submit(safe_fetch, sym): sym for sym in symbols}

        for future in as_completed(futures):
            all_options.extend(future.result())

    return all_options

def fetch_options(symbol, retry_wait=2, max_retries=10):
    print(f"\n--- {symbol} ---")

    for attempt in range(max_retries):
        try:
            ticker = yf.Ticker(symbol)
            time.sleep(1)
            expirations = ticker.options
            spot_price = ticker.info.get("regularMarketPrice", None)
            print(f"{len(expirations)} terms found for {symbol}")
            break
        except YFRateLimitError:
            print(f"[{symbol}] Rate limit reached. Attempt {attempt+1}/{max_retries}. Pause {retry_wait}s...")
            time.sleep(retry_wait)
            if attempt == 2:
                use_tor_proxy()
        except Exception as e:
            print(f"[{symbol}] Init error : {e}")
            return []
    else:
        print(f"[{symbol}] Too many errors.")
        return []

    all_options = []

    for date in expirations:
        try:
            opt_chain = ticker.option_chain(date)
            time.sleep(1)
        except Exception as e:
            print(f"[{symbol}] Date error : {date} : {e}")
            continue

        for df, opt_type in [(opt_chain.calls, "call"), (opt_chain.puts, "put")]:
            for _, row in df.iterrows():
                all_options.append({
                    "symbol": symbol,
                    "type": opt_type,
                    "strike": row["strike"],
                    "expiration": date,
                    "impliedVolatility": row.get("impliedVolatility", None),
                    "lastPrice": row.get("lastPrice", None),
                    "spot": spot_price
                })

    return all_options

def write_excel(options, filename):
    if not options:
        print("No data")
        return
    df = pd.DataFrame(options)
    df.to_excel(filename, index=False)
    print(f"{len(options)} options saved to {filename}")

def fetch_and_save(symbol):
    os.makedirs(CACHE_DIR, exist_ok=True)
    output_path = os.path.join(CACHE_DIR, f"{symbol}_ALL_options.xlsx")

    if os.path.exists(output_path):
        print(f"[{symbol}] File already exists")
        return

    options = fetch_options(symbol)
    if options:
        write_excel(options, filename=output_path)
    else:
        print(f"[{symbol}] No options found.")

async def main_async(symbols, max_workers=4):
    loop = asyncio.get_event_loop()
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        tasks = [loop.run_in_executor(executor, fetch_and_save, sym) for sym in symbols]
        await asyncio.gather(*tasks)

if __name__ == "__main__":
    uvicorn.run("fetch_data:app", host="0.0.0.0", port=8000, reload=True)

