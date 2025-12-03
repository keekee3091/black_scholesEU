#include <crow.h>
#include <nlohmann/json.hpp>
#include "black_scholes.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <random>
#include <cpr/cpr.h>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <vector>
#include  <iostream>

std::string loadEnvKey(const std::string& path = ".env") {
	std::ifstream file(path);
	std::string line;

	while (std::getline(file, line)) {
		if (line.rfind("API_KEY=", 0) == 0) {
			return line.substr(8);
		}
	}
	throw std::runtime_error("API_KEY not found in .env file");
}

nlohmann::json callPythonAPI_HTTP(const std::string& symbol)
{
	std::string apiKey;

	try {
		apiKey = loadEnvKey();
		std::cout << "[C++] Loaded API_KEY from .env: " << apiKey << std::endl;
	}
	catch (...) {
		char* key = nullptr;
		size_t len = 0;
		if (_dupenv_s(&key, &len, "API_KEY") != 0 || key == nullptr) {
			throw std::runtime_error("API_KEY not found (neither .env nor environment variable)");
		}
		apiKey = std::string(key);
		free(key);
		std::cout << "[C++] Loaded API_KEY from system env: " << apiKey << std::endl;
	}
	std::string url = "http://localhost:8000/ticker?symbol=" + symbol;
	auto response = cpr::Get(cpr::Url{ url }, cpr::Header{ {"X-API-KEY", apiKey} });
	if (response.status_code == 403) {
		throw std::runtime_error("API key rejected (403 Forbidden). Check API_KEY value.");
	}
	if (response.status_code != 200) {
		throw std::runtime_error(
			"API error (" + std::to_string(response.status_code) +
			"): " + response.text
		);
	}

	return nlohmann::json::parse(response.text);
}

nlohmann::json loadCSVToJson(const std::string& symbol) {
	std::ifstream file("C:/Dev/bs_options/black_scholesEU/Project1/cache/AAPL_historical.csv");
	if (!file.is_open()) {
		throw std::runtime_error("Unable to open CSV file.");
	}

	std::string line;
	std::getline(file, line);

	nlohmann::json json_array = nlohmann::json::array();
	while (std::getline(file, line)) {
		std::stringstream ss(line);
		std::string token;

		std::string date, act_symbol, expiration, call_put;
		double strike, bid, ask, vol, delta, gamma, theta, vega, rho;

		std::getline(ss, date, ',');
		std::getline(ss, act_symbol, ',');
		std::getline(ss, expiration, ',');
		std::getline(ss, token, ','); strike = std::stod(token);
		std::getline(ss, call_put, ',');
		std::getline(ss, token, ','); bid = std::stod(token);
		std::getline(ss, token, ','); ask = std::stod(token);
		std::getline(ss, token, ','); vol = std::stod(token);
		std::getline(ss, token, ','); delta = std::stod(token);
		std::getline(ss, token, ','); gamma = std::stod(token);
		std::getline(ss, token, ','); theta = std::stod(token);
		std::getline(ss, token, ','); vega = std::stod(token);
		std::getline(ss, token, ','); rho = std::stod(token);

		if (act_symbol != symbol) continue;

		double last_price = (bid + ask) / 2.0;

		nlohmann::json j;
		j["date"] = date;
		j["symbol"] = act_symbol;
		j["expiration"] = expiration;
		j["strike"] = strike;
		j["type"] = (call_put == "Call" ? "call" : "put");
		j["bid"] = bid;
		j["ask"] = ask;
		j["lastPrice"] = last_price;
		j["impliedVolatility"] = vol;
		j["delta"] = delta;
		j["gamma"] = gamma;
		j["theta"] = theta;
		j["vega"] = vega;
		j["rho"] = rho;
		j["volume"] = 100;

		j["spot"] = strike * 1.05;

		json_array.push_back(j);
	}

	return json_array;
}

double computeMaturity(const std::string& expiration_str) {
	std::tm tm_exp = {};
	std::istringstream ss(expiration_str);
	ss >> std::get_time(&tm_exp, "%Y-%m-%d");
	std::time_t exp_time = std::mktime(&tm_exp);
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	double T = std::difftime(exp_time, now_time) / (60 * 60 * 24 * 365.0);
	return std::max(T, 0.0);
}

double computeMaturity_Test(const std::string& expiration_str, const std::string& date_str) {
	std::tm tm_exp = {}, tm_date = {};
	std::istringstream ss_exp(expiration_str);
	std::istringstream ss_date(date_str);

	ss_exp >> std::get_time(&tm_exp, "%Y-%m-%d");
	ss_date >> std::get_time(&tm_date, "%Y-%m-%d");

	std::time_t time_exp = std::mktime(&tm_exp);
	std::time_t time_date = std::mktime(&tm_date);

	if (time_exp == -1 || time_date == -1) {
		return 0.0;
	}

	double T = std::difftime(time_exp, time_date) / (60.0 * 60 * 24 * 365.0);
	return std::max(T, 0.0);
}
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	userp->append((char*)contents, size * nmemb);
	return size * nmemb;
}

int main() {

	crow::SimpleApp app;
	CROW_ROUTE(app, "/price").methods("GET"_method)
		([](const crow::request& req) {

		const auto& qs = req.url_params;
		const char* symbol_c = qs.get("symbol");
		const char* r_c = qs.get("r");

		if (!symbol_c || !r_c)
			return crow::response(400, "missing params (symbol, r)");

		try {
			std::string symbol_query = symbol_c;
			double r = std::stod(r_c);

			// → Récupération Python
			auto data = callPythonAPI_HTTP(symbol_query);

			crow::json::wvalue results;
			results["symbol"] = symbol_query;

			std::map<std::string, std::vector<crow::json::wvalue>> grouped_options;

			// =========================================================
			// PHASE 1 : Calcul IV mean / std pour chaque symbole
			// =========================================================
			std::unordered_map<std::string, std::vector<double>> iv_by_symbol;

			for (const auto& opt : data) {
				std::string sym = opt.at("symbol").get<std::string>();
				double sigma = opt.at("impliedVolatility").get<double>();
				if (sigma > 0.0)
					iv_by_symbol[sym].push_back(sigma);
			}

			struct IVStats { double mean; double std; };
			std::unordered_map<std::string, IVStats> iv_surface;

			for (auto& [sym, vols] : iv_by_symbol) {
				double sum = 0.0;
				for (double v : vols) sum += v;
				double mean = sum / vols.size();

				double sq = 0.0;
				for (double v : vols) sq += (v - mean) * (v - mean);
				double std = std::sqrt(sq / vols.size());
				if (std < 0.0001) std = 0.0001;

				iv_surface[sym] = { mean, std };
			}

			// =========================================================
			// PARAMÈTRES RÉALISTES
			// =========================================================
			const double MIN_VOLUME = 50;
			const double MIN_DELTA = 0.02;
			const double MAX_DELTA = 0.98;
			const double MIN_PROB = 0.10;
			const double MAX_PROB = 0.98;

			// BUY / SELL réalistes
			const double BUY_SCORE = 10;
			const double SELL_SCORE = -10;


			// =========================================================
			// PHASE 2 : CALCULS et SCORING
			// =========================================================

			for (const auto& opt : data) {

				std::string opt_type = opt.at("type").get<std::string>();
				std::string sym = opt.at("symbol").get<std::string>();

				double K = opt.at("strike").get<double>();
				double S = opt.at("spot").get<double>();
				double sigma = opt.at("impliedVolatility").get<double>();
				double last_px = opt.at("lastPrice").get<double>();
				std::string expiration = opt.at("expiration").get<std::string>();
				double T = computeMaturity(expiration);
				double V = opt.contains("volume") ? opt.at("volume").get<double>() : 100.0;

				if (T <= 0 || sigma <= 0 || S <= 0) continue;
				if (sigma < 0.01) continue;

				auto stats = iv_surface[sym];
				double iv_mean = stats.mean;
				double iv_std = stats.std;

				double bs_price = (opt_type == "call")
					? blackScholesCall(S, K, r, sigma, T)
					: blackScholesPut(S, K, r, sigma, T);

				double delta = (opt_type == "call")
					? deltaCall(S, K, r, sigma, T)
					: deltaPut(S, K, r, sigma, T);

				double gam = gamma(S, K, r, sigma, T);
				double theta = (opt_type == "call") ? thetaCall(S, K, r, sigma, T)
					: thetaPut(S, K, r, sigma, T);
				double rho_val = (opt_type == "call") ? rhoCall(S, K, r, sigma, T)
					: rhoPut(S, K, r, sigma, T);
				double vega_val = vega(S, K, r, sigma, T);

				double prob_ITM = brownProb(S, K, r, sigma, T, opt_type);
				double mispricing = bs_price - last_px;

				double raw_score = mispricing * delta * V;
				if (std::isnan(raw_score)) continue;

				// -----------------------------------------------------
				// MÉTRIQUES OPTIMISÉES
				// -----------------------------------------------------
				double vega_score = vegaNormalized(mispricing, vega_val);
				double iv_z = ivZScore(sigma, iv_mean, iv_std);
				double liq = liquidityScore(V, delta);
				double gammaRisk = gammaRiskScore(gam, S, 0.02);

				double skewVal = skewEdge(sigma, iv_mean, iv_std, S / K);
				double smileDist = smileDistance(sigma, iv_mean);

				double final_score = sabrEnhancedScore(
					raw_score,
					vega_score,
					iv_z,
					liq,
					gammaRisk,
					skewVal,
					smileDist
				);
				// ====================
				// BS VALIDITY FILTERS
				// ====================
				bool extremeITM = (opt_type == "call" && S / K > 2.5)
					|| (opt_type == "put" && K / S > 2.5);

				bool shortMaturity = (T < 0.03);

				bool inconsistentProb =
					(opt_type == "call" && S > 1.5 * K && prob_ITM < 0.70)
					|| (opt_type == "put" && K > 1.5 * S && prob_ITM < 0.70);

				// Cap BS price for ITM near-expiry
				double intrinsic = std::max((opt_type == "call" ? S - K : K - S), 0.0);
				if (extremeITM && T < 0.10) {
					bs_price = intrinsic + 2.0;
					mispricing = bs_price - last_px;
				}

				// Stabilized theta
				double theta_adj = -(last_px - intrinsic) / T;
				if (extremeITM || shortMaturity)
					theta = std::max(theta, theta_adj);
				// -----------------------------------------------------
				// RÈGLES DE TRADING
				// -----------------------------------------------------
				std::string action = "hold";
				std::string reason = "Neutral";

				bool bad_maturity = (T < 0.02);
				bool bad_volume = (V < MIN_VOLUME);

				double max_delta_allowed = dynamicMaxDelta(T, (S / K));
				bool bad_delta = (std::abs(delta) < MIN_DELTA || std::abs(delta) > max_delta_allowed);

				bool bad_prob = (prob_ITM < MIN_PROB || prob_ITM > MAX_PROB);

				if (bad_maturity || bad_volume || bad_delta || bad_prob) {
					action = "ignore";
					reason = "Market structure filter";
				}
				else if (final_score > BUY_SCORE && vega_score > 0.05 && mispricing > 0 && ((iv_z < 0.15) || (delta > 0.75 && gammaRisk < 0.5))) {
					action = "buy";
					reason = "Strong ITM + positive edge despite elevated IV";
				}
				else if (mispricing < 0 && final_score < SELL_SCORE && vega_score < -0.05 && iv_z > 0) {
					action = "sell";
					reason = "Expensive IV + negative vega edge + good structure";
				}

				// -----------------------------------------------------
				// JSON OUTPUT
				// -----------------------------------------------------
				crow::json::wvalue res;
				res["type"] = opt_type;
				res["strike"] = K;
				res["spot"] = S;
				res["expiration"] = expiration;
				res["maturity"] = T;
				res["sigma"] = sigma;

				res["bs_price"] = bs_price;
				res["market_price"] = last_px;

				res["delta"] = delta;
				res["gamma"] = gam;
				res["theta"] = theta;
				res["vega"] = vega_val;
				res["rho"] = rho_val;

				res["volume"] = V;
				res["mispricing"] = mispricing;
				res["prob_ITM"] = prob_ITM;
				res["moneyness"] = S / K;

				res["iv_mean"] = iv_mean;
				res["iv_std"] = iv_std;
				res["iv_z"] = iv_z;

				res["vega_score"] = vega_score;
				res["liquidity"] = liq;
				res["gamma_risk"] = gammaRisk;

				res["final_score"] = final_score;
				res["action"] = action;
				res["action_reason"] = reason;

				grouped_options[sym].push_back(std::move(res));
			}

			// =========================================================
			// SÉRIALISATION FINALE
			// =========================================================

			crow::json::wvalue options_by_symbol;
			for (auto& [sym, vec] : grouped_options)
				options_by_symbol[sym] = std::move(vec);

			results["options"] = std::move(options_by_symbol);
			return crow::response{ results };
		}
		catch (const std::exception& e) {
			return crow::response(500, std::string("Internal error: ") + e.what());
		}
			});

	CROW_ROUTE(app, "/historical").methods("GET"_method)
		([](const crow::request& req) {
		const auto& qs = req.url_params;
		const char* symbol_c = qs.get("symbol");
		const char* r_c = qs.get("r");

		if (!symbol_c || !r_c)
			return crow::response(400, "missing params (symbol, r)");

		try {
			std::string symbol_query = symbol_c;

			double r = std::stod(r_c);
			auto data = loadCSVToJson(symbol_query);
			// renvoie toutes les options

			crow::json::wvalue results;
			results["symbol"] = symbol_query;

			std::map<std::string, std::vector<crow::json::wvalue>> grouped_options;

			const double score_threshold = 0.1;
			const double min_volume = 100;
			const double score_buy_thresold = 300.0;
			const double score_sell_threshold = -300.0;
			const double prob_ITM_threshold = 0.6;
			const double volume_threshold = 100.0;
			const double delta_limit = 0.2;

			for (const auto& opt : data) {
				std::string opt_type = opt.at("type").get<std::string>();
				std::string sym = opt.at("symbol").get<std::string>();
				double K = opt.at("strike").get<double>();
				double S = opt.at("spot").get<double>();
				double sigma = opt.at("impliedVolatility").get<double>();
				if (sigma < 0.01) continue;
				double last_price = opt.at("lastPrice").get<double>();
				std::string expiration = opt.at("expiration").get<std::string>();
				std::string date = opt.at("date").get<std::string>();
				double T = computeMaturity_Test(expiration, date);
				double V = opt.contains("volume") ? opt.at("volume").get<double>() : 100.0;

				if (T <= 0.0 || sigma <= 0.0 || S <= 0.0) continue;

				double bs_price = (opt_type == "call")
					? blackScholesCall(S, K, r, sigma, T)
					: blackScholesPut(S, K, r, sigma, T);

				double delta = (opt_type == "call")
					? deltaCall(S, K, r, sigma, T)
					: deltaPut(S, K, r, sigma, T);

				double gam = gamma(S, K, r, sigma, T);
				double theta = (opt_type == "call")
					? thetaCall(S, K, r, sigma, T)
					: thetaPut(S, K, r, sigma, T);
				double rho = (opt_type == "call")
					? rhoCall(S, K, r, sigma, T)
					: rhoPut(S, K, r, sigma, T);
				double vega_val = vega(S, K, r, sigma, T);

				double prob_ITM = brownProb(S, K, r, sigma, T, opt_type);
				double mispricing = bs_price - last_price;
				double score = mispricing * delta * V;

				if (std::isnan(score) || std::isnan(prob_ITM)) continue;

				std::string action = "hold";

				std::string reason = "Moderate score";

				if (V < volume_threshold || T < 0.1) {
					action = "ignore";
					reason = "Low volume or short maturity";
				}
				else if (score >= score_buy_thresold && prob_ITM >= prob_ITM_threshold && std::abs(delta) > delta_limit) {
					action = "buy";
					reason = "High score + strong ITM probability + significant delta";
				}
				else if (score <= score_sell_threshold && prob_ITM >= prob_ITM_threshold && std::abs(delta) > delta_limit) {
					action = "sell";
					reason = "Strong negative score + strong ITM probability + significant delta";
				}

				crow::json::wvalue res;
				res["type"] = opt_type;
				res["strike"] = K;
				res["spot"] = S;
				res["expiration"] = expiration;
				res["maturity"] = T;
				res["sigma"] = sigma;
				res["bs_price"] = bs_price;
				res["market_price"] = last_price;
				res["delta"] = delta;
				res["gamma"] = gam;
				res["theta"] = theta;
				res["vega"] = vega_val;
				res["rho"] = rho;
				res["volume"] = V;
				res["mispricing"] = mispricing;
				res["score"] = score;
				res["prob_ITM"] = prob_ITM;
				res["moneyness"] = S / K;
				res["action"] = action;
				res["action_reason"] = reason;

				grouped_options[sym].push_back(std::move(res));
			}

			// Convertir grouped_options en crow::json::wvalue
			crow::json::wvalue options_by_symbol;
			for (auto& [sym, vec] : grouped_options) {
				options_by_symbol[sym] = std::move(vec);
			}

			results["options"] = std::move(options_by_symbol);
			return crow::response{ results };
		}
		catch (const std::exception& e) {
			return crow::response(500, std::string("Intern error : ") + e.what());
		}
			});

	app.port(8080).multithreaded().run();


	return 0;
}
