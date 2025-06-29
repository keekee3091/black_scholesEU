# 📈 Option Pricing & Stock Valuation API

## 🔧 Description

Cette API en **C++** basée sur **Crow** permet :

- de **calculer le prix d'une option** (`call` ou `put`) selon le modèle de **Black-Scholes** ;
- de déterminer le **point mort (_break-even_)** ;
- d’estimer la **probabilité d’atteindre un objectif de prix** via un processus de **Brownien géométrique** ;
- d’analyser la **valorisation d’une action** (_sous-évaluation_ ou _surévaluation_) à l’aide de **ratios financiers fondamentaux**.

---

## 🚀 Endpoints

### 🔹 `/price`

Calcule le prix d'une option Black-Scholes et renvoie plusieurs métriques.

#### 📥 Paramètres :

| Paramètre | Type   | Description                                 |
|-----------|--------|---------------------------------------------|
| `symbol`  | string | Ticker de l'action (ex : `AAPL`)            |
| `K`       | double | Prix d'exercice (_strike_)                  |
| `r`       | double | Taux sans risque                            |
| `T`       | double | Temps jusqu’à l’échéance (en années)        |
| `type`    | string | `"call"` ou `"put"`                         |
| `target`  | double *(optionnel)* | Objectif de prix à atteindre |

#### 📤 Exemple de réponse :

```json
{
  "symbol": "AAPL",
  "S": 201.08,
  "price": 125.57,
  "type": "call",
  "sigma": 0.49,
  "break_even": 205.57,
  "target": 210,
  "net gain/loss": 4.42,
  "is_profitable": true,
  "probability_target_hit": 0.406
}
```

---

### 🔹 `/valuation`

Retourne les **ratios fondamentaux** et évalue la **valorisation** d’un actif.

#### 📥 Paramètres :

| Paramètre | Type   | Description                              |
|-----------|--------|------------------------------------------|
| `symbol`  | string | Ticker d’une action (ex : `AAPL`, `TSLA`) |

#### 📤 Exemple de réponse :

```json
{
  "symbol": "AAPL",
  "valuation_status": "Surévaluée",
  "pe_ratio": 31.2,
  "pb_ratio": 42.7,
  "peg_ratio": 2.8,
  "roe": 0.77,
  "dividend_yield": 0.0056
}
```

---

## 📦 Dépendances

- [`Crow`](https://github.com/CrowCpp/crow) – microframework web C++
- [`cpr`](https://github.com/libcpr/cpr) – HTTP client
- [`nlohmann/json`](https://github.com/nlohmann/json) – JSON parsing
- `OpenSSL` – requis si HTTPS

---

## ⚙️ Compilation

```bash
g++ main.cpp -o app -lcpr -lpthread -lssl -lcrypto
./app
```
