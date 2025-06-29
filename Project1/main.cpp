#include <crow.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "black_scholes.hpp"

double getLivePrice(const std::string& symbol) {
	auto response = cpr::Get(
		cpr::Url{ "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol },
		cpr::Header{ { "User-Agent", "Mozilla/5.0" } }
	);

	if (response.status_code != 200) {
		throw std::runtime_error("Erreur API Yahoo : HTTP " + std::to_string(response.status_code));
	}

	auto json = nlohmann::json::parse(response.text);
	return json["chart"]["result"][0]["meta"]["regularMarketPrice"];
}


int main() {
	crow::SimpleApp app;

	CROW_ROUTE(app, "/price").methods("GET"_method)
		([](const crow::request& req) {
		const auto& qs = req.url_params;

		try {
			const char* symbol_c = qs.get("symbol");
			const char* K_c = qs.get("K");
			const char* r_c = qs.get("r");
			const char* sigma_c = qs.get("sigma");
			const char* T_c = qs.get("T");
			const char* type_c = qs.get("type");

			if (!symbol_c || !K_c || !r_c || !sigma_c || !T_c || !type_c) {
				return crow::response(400, "Erreur : Paramètre manquant");
			}

			std::string symbol = symbol_c;
			double K = std::stod(K_c);
			double r = std::stod(r_c);
			double sigma = std::stod(sigma_c);
			double T = std::stod(T_c);
			std::string type = type_c;

			double S = getLivePrice(symbol);

			double result = (type == "call")
				? blackScholesCall(S, K, r, sigma, T)
				: blackScholesPut(S, K, r, sigma, T);

			crow::json::wvalue res;
			res["symbol"] = symbol;
			res["S"] = S;
			res["price"] = result;
			res["type"] = type;
			return crow::response{ res };
		}
		catch (const std::exception& e) {
			return crow::response(500, std::string("Erreur interne : ") + e.what());
		}
			});

	std::cout << "Serveur lancé sur http://localhost:8080/price" << std::endl;
	app.port(8080).multithreaded().run();
}
