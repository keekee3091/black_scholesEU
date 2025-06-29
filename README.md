📈 Option Pricing & Stock Valuation API
🔧 Description
Cette API en C++ basée sur Crow permet :

de calculer le prix d'une option (call ou put) selon le modèle de Black-Scholes ;

de déterminer le point mort (break-even) ;

d’estimer la probabilité de toucher un objectif via un processus de Brownien géométrique ;

d’analyser la valorisation d’une action (sous-évaluation ou surévaluation) via les ratios financiers fondamentaux.

🚀 Endpoints
🔹 /price
Calcule le prix d'une option Black-Scholes et renvoie plusieurs métriques.

📥 Paramètres :
Paramètre	Type	Description
symbol	string	Ticker de l'action (ex : AAPL)
K	double	Prix d'exercice (strike)
r	double	Taux sans risque
T	double	Temps jusqu’à l’échéance (en années)
type	string	"call" ou "put"
target (optionnel)	double	Objectif de prix à atteindre

📤 Exemple de réponse :
json
Copier
Modifier
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
🔹 /valuation
Retourne les ratios fondamentaux et évalue la valorisation d’un actif.

📥 Paramètres :
Paramètre	Type	Description
symbol	string	Ticker d’une action (AAPL, TSLA, etc.)

📤 Exemple de réponse :
json
Copier
Modifier
{
  "symbol": "AAPL",
  "valuation_status": "Surévaluée",
  "pe_ratio": 31.2,
  "pb_ratio": 42.7,
  "peg_ratio": 2.8,
  "roe": 0.77,
  "dividend_yield": 0.0056
}
📦 Dépendances
Crow – microframework web C++

cpr – HTTP client

nlohmann/json – JSON parsing

OpenSSL (si HTTPS requis)

⚙️ Compilation
bash
Copier
Modifier
g++ main.cpp -o app -lcpr -lpthread -lssl -lcrypto
./app
