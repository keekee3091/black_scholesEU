#include <cmath>
#include "black_scholes.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double normalCDF(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2));
}

double normalPDF(double x) {
    return (1.0 / std::sqrt(2 * M_PI)) * std::exp(-0.5 * x * x);
}

double blackScholesCall(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    double d2 = d1 - sigma * std::sqrt(T);
    return S * normalCDF(d1) - K * std::exp(-r * T) * normalCDF(d2);
}

double blackScholesPut(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    double d2 = d1 - sigma * std::sqrt(T);
    return K * std::exp(-r * T) * normalCDF(-d2) - S * normalCDF(-d1);
}

double deltaCall(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    return normalCDF(d1);
}

double deltaPut(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    return normalCDF(d1) - 1.0;
}

double gamma(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    return normalPDF(d1) / (S * sigma * std::sqrt(T));
}

double vega(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    return S * normalPDF(d1) * std::sqrt(T);
}

double thetaCall(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    double d2 = d1 - sigma * std::sqrt(T);
    double term1 = -(S * normalPDF(d1) * sigma) / (2 * std::sqrt(T));
    double term2 = -r * K * std::exp(-r * T) * normalCDF(d2);
    return term1 + term2;
}

double thetaPut(double S, double K, double r, double sigma, double T) {
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    double d2 = d1 - sigma * std::sqrt(T);
    double term1 = -(S * normalPDF(d1) * sigma) / (2 * std::sqrt(T));
    double term2 = +r * K * std::exp(-r * T) * normalCDF(-d2);
    return term1 + term2;
}

double rhoCall(double S, double K, double r, double sigma, double T) {
    double d2 = (std::log(S / K) + (r - 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    return K * T * std::exp(-r * T) * normalCDF(d2);
}

double rhoPut(double S, double K, double r, double sigma, double T) {
    double d2 = (std::log(S / K) + (r - 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    return -K * T * std::exp(-r * T) * normalCDF(-d2);
}

double brownProb(double S, double target, double r, double sigma, double T, const std::string& type) {
    if (target <= 0 || sigma <= 0 || T <= 0) return 0.0;

    double d = (std::log(target / S) - (r - 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));

    if (type == "call") {
        return 1.0 - normalCDF(d);
    }
    else {
        return normalCDF(d);
    }
}
