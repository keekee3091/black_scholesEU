# Projet Black-Scholes avec API Python et Serveur C++

## Description

Ce projet combine une API Python utilisant yfinance et FastAPI pour récupérer les données d’options financières, et un serveur C++ (avec Crow) qui interroge cette API pour calculer les prix d’options européennes via le modèle de Black-Scholes.  
Le serveur C++ réalise également des calculs de grecs (Delta, Gamma) et d’autres indicateurs financiers.

---

## Structure du projet

### Partie Python (fetch_data.py)

- Récupération des données d’options pour un symbole donné via yfinance.
- Mise en cache locale dans le dossier cache/ sous forme de fichiers Excel.
- Gestion des limitations d’API avec possibilité d’utiliser un proxy TOR (127.0.0.1:9050).
- API REST avec FastAPI exposant l’endpoint /ticker?symbol=XXX pour retourner les options au format JSON.

### Partie C++ (main.cpp)

- Serveur HTTP avec Crow sur le port 8080.
- Appels au script Python via Python C API pour récupérer les données d’options.
- Calcul des prix d’options avec le modèle Black-Scholes (calls et puts).
- Calcul des grecs (Delta, Gamma) et d’un score de mispricing.
- Filtrage des options selon score et volume.

---

## Fonctionnalités clés

- Requêtes asynchrones multithreadées en Python.
- Cache local pour optimiser les performances.
- Proxy TOR automatique en cas de dépassement de quota.
- Serveur C++ performant pour calculs financiers complexes.
- Interface REST simple pour obtenir les prix d’options corrigés.

---

## Installation & Prérequis

### Python

- Python 3.8+
- Installer les packages :

  pip install yfinance pandas fastapi uvicorn requests openpyxl

- TOR (optionnel) doit être installé et en cours d’exécution si le proxy TOR est utilisé.

### C++

- Compilateur C++17 ou supérieur.
- Bibliothèques nécessaires :
  - Crow (serveur HTTP)
  - nlohmann/json (JSON)
  - Headers et libs de Python (Python C API)
- Compiler en liant les bibliothèques Python et Crow.

---

## Usage

### Lancer l’API Python

uvicorn fetch_data:app --host 0.0.0.0 --port 8000 --reload

### Lancer le serveur C++

Compiler et exécuter le serveur. Exemple de requête :

GET http://localhost:8080/price?symbol=AAPL&r=0.01

---

## Modèle Black-Scholes

Le calcul du prix des options se base sur :

- blackScholesCall(S, K, r, sigma, T)
- blackScholesPut(S, K, r, sigma, T)

avec :

- S = prix du sous-jacent
- K = prix d’exercice
- r = taux sans risque
- sigma = volatilité implicite
- T = maturité (en années)

Les grecs calculés sont Delta (deltaCall, deltaPut) et Gamma (gamma).

---

## Proxy TOR

En cas de rate limiting sur yfinance, le script Python active un proxy TOR local (socks5h://127.0.0.1:9050) pour contourner la limitation.

---

## Caching

Les données d’options sont sauvegardées dans le dossier cache/ sous forme de fichiers Excel <SYMBOL>_ALL_options.xlsx afin d’éviter des appels redondants à l’API yfinance.

---

## Auteur & Licence

- Auteur : weeb_gang3091
- Licence : MIT

---

## Notes

- Adapter le chemin Python dans le code C++ (sys.path.append) selon ton environnement.  
- TOR doit être installé et configuré si proxy utilisé.  
- Les performances dépendent du nombre de requêtes simultanées et des quotas yfinance.
