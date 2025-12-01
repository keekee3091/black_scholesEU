#include <Python.h>
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

nlohmann::json callPythonAPI_HTTP(const std::string& symbol) {
    std::string url = "http://localhost:8000/ticker?symbol=" + symbol;
    auto response = cpr::Get(cpr::Url{ url });

    if (response.status_code != 200) {
        throw std::runtime_error("API error: " + response.text);
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
            std::string symbol_query = symbol_c;  // "JNJ,XOM,PG,V"
            double r = std::stod(r_c);
            auto data = callPythonAPI_HTTP(symbol_query);  // renvoie toutes les options

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
                double T = computeMaturity(expiration);
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
