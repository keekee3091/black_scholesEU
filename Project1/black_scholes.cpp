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

double vegaNormalized(double mispricing, double vegaVal) {
    if (std::abs(vegaVal) < 0.5) return 0.0;   
    return mispricing / vegaVal;
}


double ivZScore(double sigma, double ivMean, double ivStd) {
    if (ivStd < 1e-9) return 0.0;
    return (sigma - ivMean) / ivStd;
}

double gammaRiskScore(double gammaVal, double spot, double movePercent = 0.02) {
    double dS = spot * movePercent;
    return std::abs(gammaVal) * dS * dS;
}

double thetaBleedDaily(double thetaVal) {
    return thetaVal;
}

double liquidityScore(double volume, double delta) {
    return std::sqrt(volume) * std::abs(delta);
}

double skewEdge(double sigma, double iv_mean, double iv_std, double moneyness) {
    if (iv_std < 1e-9) return 0.0;
    double nz = (sigma - iv_mean) / iv_std;
    return -nz * (moneyness - 1.0);
}

double smileDistance(double sigma, double iv_mean) {
    return iv_mean - sigma;
}

double sabrEnhancedScore(
    double scoreRaw,
    double vegaNorm,
    double ivZ,
    double liquidity,
    double gamRisk,
    double skew,
    double smileDist)
{
    return 0.6 * scoreRaw
        + 1.0 * vegaNorm
        - 0.3 * std::abs(ivZ)
        + 0.8 * liquidity
        - 0.5 * gamRisk
        + 0.4 * skew
        + 0.2 * smileDist;
}

double dynamicMaxDelta(double T, double moneyness) {
    double base = 0.95 + 0.03 * std::exp(-5 * T);
    if (moneyness > 2.0)  
        base += 0.02;     
    return std::min(base, 0.995);
}