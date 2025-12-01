#include <iostream>
#include <cmath>
#include <string>
#include <cassert>
#include "black_scholes.hpp"


double blackScholesCall(double S, double K, double r, double sigma, double T);
double blackScholesPut(double S, double K, double r, double sigma, double T);
double brownProb(double S, double K, double r, double sigma, double T, const std::string& type);

void testOption(const std::string& opt_type, double S, double K, double r, double sigma, double T, double market_price) {
    double bs_price = (opt_type == "Call") ? blackScholesCall(S, K, r, sigma, T) : blackScholesPut(S, K, r, sigma, T);
    double prob_ITM = brownProb(S, K, r, sigma, T, opt_type);

    bool coherent = true;
    if (opt_type == "Call") {
        if (prob_ITM < 0.5 || bs_price < market_price)
            coherent = false;
    }
    else {
        if (prob_ITM > 0.5 || bs_price > market_price)
            coherent = false;
    }

    if (!coherent) {
        std::cerr << "Test incoherent pour option " << opt_type << ", strike=" << K
            << ", market_price=" << market_price << ", bs_price=" << bs_price
            << ", prob_ITM=" << prob_ITM << std::endl;
    }

    assert(coherent); 
}

int main_test() {
    std::string opt_type = "Call";
    double S = 150.0;           
    double K = 145.0;            // Strike
    double r = 0.01;             // Taux sans risque annuel
    double sigma = 0.42;         // Volatilité implicite
    double T = 13.0 / 365.0;     // Maturité en années (ex: 13 jours)
    double market_price = 25.3;  // Prix du marché

    testOption(opt_type, S, K, r, sigma, T, market_price);


    std::cout << "Tous les tests unitaires ont réussi." << std::endl;
    return 0;
}
