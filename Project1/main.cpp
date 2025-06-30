#include <crow.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "black_scholes.hpp"
#include <numeric>
#include <cmath>

double getLivePrice(const std::string& symbol) {
	auto responses = cpr::Get(
		cpr::Url{ "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol },
		cpr::Header{ { "User-Agent", "Mozilla/5.0" } }
	);

	if (responses.status_code != 200) {
		throw std::runtime_error("Erreur API Yahoo : HTTP " + std::to_string(responses.status_code));
	}

	auto json = nlohmann::json::parse(responses.text);
	return json["chart"]["result"][0]["meta"]["regularMarketPrice"];
}

double calculateSigma(const std::string& symbol) {
	auto responses = cpr::Get(
		cpr::Url{ "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol + "?range=60d&interval=1d" },
		cpr::Header{ { "User-Agent", "Mozilla/5.0"} }
	);
	if (responses.status_code != 200) {
		throw std::runtime_error("Erreur API Yahoo : HTTP " + std::to_string(responses.status_code));
	}
	auto json = nlohmann::json::parse(responses.text);
	auto close = json["chart"]["result"][0]["indicators"]["quote"][0]["close"];

	std::vector<double> returns;
	for (size_t i = 1; i < close.size(); ++i) {
		if (!close[i].is_null() && !close[i - 1].is_null()) {
			double r = std::log(close[i].get<double>() / close[i - 1].get<double>());
			returns.push_back(r);
		}
	}
	if (returns.empty()) {
		throw std::runtime_error("Erreur API Yahoo : HTTP " + std::to_string(responses.status_code));
	}
	double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
	double variance = 0.0;
	for (const auto& r : returns) {
		variance += (r - mean) * (r - mean);
	}
	variance /= returns.size();
	double sigma = std::sqrt(variance) * std::sqrt(252.0);
	return sigma;
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
			const char* T_c = qs.get("T");
			const char* type_c = qs.get("type");
			const char* target_c = qs.get("target");

			if (!symbol_c || !K_c || !r_c || !T_c || !type_c) {
				return crow::response(400, "Erreur : Paramètre manquant");
			}

			std::string symbol = symbol_c;
			double K = std::stod(K_c);
			double r = std::stod(r_c);
			double T = std::stod(T_c);
			std::string type = type_c;
			double target = target_c ? std::stod(target_c) : -1.0;

			double sigma = calculateSigma(symbol);
			double S = getLivePrice(symbol);

			double result = (type == "call")
				? blackScholesCall(S, K, r, sigma, T)
				: blackScholesPut(S, K, r, sigma, T);

			double break_even = (type == "call") ? K + result : K - result;

			double net = 0.0;
			bool is_profitable = false;
			if (target > 0) {
				net = (type == "call") ? std::max(target - K, 0.0) - result
					: std::max(K - target, 0.0) - result;
				is_profitable = (type == "call") ? (target > break_even)
					: (target < break_even);
			}

			double prob = (target > 0) ? brownProb(S, target, r, sigma, T, type) : -1.0;

			crow::json::wvalue res;
			res["symbol"] = symbol;
			res["S"] = S;
			res["price"] = result;
			res["type"] = type;
			res["sigma"] = sigma;
			res["break_even"] = break_even;

			if (target > 0) {
				res["target"] = target;
				res["net gain/loss"] = net;
				res["is_profitable"] = is_profitable;
				res["probability_target_hit"] = prob;
			}

			return crow::response{ res };
		}
		catch (const std::exception& e) {
			return crow::response(500, std::string("Erreur interne : ") + e.what());
		}
			});


	std::cout << "Serveur lancé sur http://localhost:8080/" << std::endl;
	app.port(8080).multithreaded().run();
}
