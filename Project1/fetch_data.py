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

CACHE_DIR = "cache"
app = FastAPI()

@app.get('/ticker')
async def get_ticker(symbol: str = Query(..., description="Le ticker à rechercher (ex: AAPL, MSFT)")):
    try:
        await main_async([symbol.upper()])
        return {"message": f"Traitement terminé pour {symbol.upper()}"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Erreur interne : {str(e)}")

def use_tor_proxy():
    print("[TOR] Utilisation du proxy socks5h://127.0.0.1:9050")

    class TorSession(requests.Session):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.proxies.update({
                'http': 'socks5h://127.0.0.1:9050',
                'https': 'socks5h://127.0.0.1:9050'
            })

    requests.Session = TorSession

def fetch_options(symbol, retry_wait=10, max_retries=10):
    print(f"\n--- {symbol} ---")
    time.sleep(2)

    for attempt in range(max_retries):
        try:
            ticker = yf.Ticker(symbol)
            time.sleep(1)
            expirations = ticker.options
            spot_price = ticker.info.get("regularMarketPrice", None)
            print(f"{len(expirations)} échéances trouvées pour {symbol}")
            break
        except YFRateLimitError:
            print(f"[{symbol}] Rate limit atteint. Tentative {attempt+1}/{max_retries}. Pause {retry_wait}s...")
            time.sleep(retry_wait)
            if attempt == 2:
                use_tor_proxy()
        except Exception as e:
            print(f"[{symbol}] Erreur initiale : {e}")
            return []
    else:
        print(f"[{symbol}] Trop de tentatives infructueuses. Abandon.")
        return []

    all_options = []

    for date in expirations:
        try:
            opt_chain = ticker.option_chain(date)
            time.sleep(1)
        except Exception as e:
            print(f"[{symbol}] Erreur à la date {date} : {e}")
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
        print("Aucune donnée à écrire.")
        return
    df = pd.DataFrame(options)
    df.to_excel(filename, index=False)
    print(f"{len(options)} options saved to {filename}")

def fetch_and_save(symbol):
    os.makedirs(CACHE_DIR, exist_ok=True)
    output_path = os.path.join(CACHE_DIR, f"{symbol}_ALL_options.xlsx")

    if os.path.exists(output_path):
        print(f"[{symbol}] Fichier déjà existant, on saute.")
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

