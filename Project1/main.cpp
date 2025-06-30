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


nlohmann::json callPythonAPI(const std::string& symbol) {
    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('C:/Dev/bs_options/black_scholesEU/Project1')");
    nlohmann::json result_json;

    try {
        PyObject* pName = PyUnicode_FromString("fetch_data");
        PyObject* pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if (!pModule) {
            PyErr_Print();  // <== Ajoute ceci pour voir l'erreur réelle
            throw std::runtime_error("Module Python non chargé");
        }
        PyObject* pFunc = PyObject_GetAttrString(pModule, "get_ticker");
        if (!pFunc || !PyCallable_Check(pFunc)) throw std::runtime_error("Fonction get_ticker non trouvée");

        PyObject* pArgs = PyTuple_Pack(1, PyUnicode_FromString(symbol.c_str()));
        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
        Py_DECREF(pArgs);
        if (!pResult) throw std::runtime_error("Erreur lors de l'appel Python");

        PyObject* jsonModule = PyImport_ImportModule("json");
        PyObject* dumpsFunc = PyObject_GetAttrString(jsonModule, "dumps");
        PyObject* jsonArgs = PyTuple_Pack(1, pResult);
        PyObject* kwargs = PyDict_New();
        PyDict_SetItemString(kwargs, "ensure_ascii", Py_True); 
        PyObject* jsonStr = PyObject_Call(dumpsFunc, jsonArgs, kwargs);
        Py_DECREF(kwargs);

        const char* json_cstr = PyUnicode_AsUTF8(jsonStr);
        result_json = nlohmann::json::parse(json_cstr);

        Py_XDECREF(jsonStr); Py_XDECREF(jsonArgs); Py_XDECREF(dumpsFunc);
        Py_XDECREF(jsonModule); Py_XDECREF(pResult); Py_XDECREF(pFunc); Py_XDECREF(pModule);
    }
    catch (...) {
        Py_Finalize();
        throw;
    }

    Py_Finalize();
    return result_json;
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

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/price").methods("GET"_method)
        ([](const crow::request& req) {
        const auto& qs = req.url_params;
        const char* symbol_c = qs.get("symbol");
        const char* r_c = qs.get("r");

        if (!symbol_c || !r_c)
            return crow::response(400, "Paramètre manquant (symbol, r)");

        try {
            std::string symbol = symbol_c;
            double r = std::stod(r_c);

            auto data = callPythonAPI(symbol);

            crow::json::wvalue results;
            results["symbol"] = symbol;
            std::vector<crow::json::wvalue> opt_list;

            const double score_threshold = 0.1;
            const double min_volume = 100;

            for (const auto& opt : data) {
                std::string opt_type = opt["type"];
                double K = opt["strike"];
                double S = opt["spot"];
                double sigma = opt["impliedVolatility"];
                if (sigma < 0.01) continue;
                double last_price = opt["lastPrice"];
                std::string expiration = opt["expiration"];
                double T = computeMaturity(expiration);
                double V = opt.value("volume", 100.0);

                if (T <= 0.0 || sigma <= 0.0 || S <= 0.0) continue;

                double bs_price = (opt_type == "call")
                    ? blackScholesCall(S, K, r, sigma, T)
                    : blackScholesPut(S, K, r, sigma, T);

                double delta = (opt_type == "call")
                    ? deltaCall(S, K, r, sigma, T)
                    : deltaPut(S, K, r, sigma, T);

                double gam = gamma(S, K, r, sigma, T);

                double prob_ITM = brownProb(S, K, r, sigma, T, opt_type);

                double mispricing = bs_price - last_price;

                double score = mispricing * delta * V;

                if (std::abs(score) < score_threshold || V < min_volume) continue;

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
                res["volume"] = V;
                res["mispricing"] = mispricing;
                res["score"] = score;
                res["prob_ITM"] = prob_ITM;

                opt_list.emplace_back(std::move(res));
            }

            results["options"] = std::move(opt_list);
            return crow::response{ results };
        }
        catch (const std::exception& e) {
            return crow::response(500, std::string("Erreur interne : ") + e.what());
        }
            });
    app.port(8080).multithreaded().run();
    return 0;
}
