# Black-Scholes Scanner — API Python + Serveur C++

Ce projet combine une API Python (FastAPI) pour la collecte des données d’options et un serveur C++ (Crow) pour effectuer des calculs financiers haute performance : modèle de Black-Scholes, grecs, statistiques de volatilité et scoring avancé.

L’ensemble constitue un scanner d’options complet, utilisable en local ou comme microservice.

---

## 📌 Fonctionnalités principales

### Partie Python (FastAPI)
- Récupération des chaînes d’options via **yfinance**.
- Mise en cache automatique dans des fichiers Excel.
- Contournement du rate-limit via un proxy **TOR** en fallback.
- Deux endpoints principaux :
  - `GET /ticker?symbol=XYZ` → renvoie toutes les options du jour.
  - `GET /historical?symbol=XYZ&from=YYYY-MM-DD&to=YYYY-MM-DD` → prix historiques.
- Sécurisation par **clé API obligatoire** (`X-API-KEY`).
- Chargement automatique des variables depuis `.env`.

### Partie C++ (Crow)
- Serveur HTTP léger et performant.
- Appel à l’API Python via **cpr** avec clé API.
- Calculs :
  - Black-Scholes (call/put)
  - Delta, Gamma, Theta, Vega, Rho
  - Probabilité d’expiration ITM
  - Scores avancés (mispricing, vega-normalized, IV z-score, gamma risk, skew, smile, score SABR-like)
- Logique de filtrage :
  - maturité minimale
  - volume minimal
  - delta admissible
  - probabilité ITM raisonnable
  - rejet structurel (`action = "ignore"`)
- Retour Au format JSON pour intégration dans un frontend (ex. Blazor).
