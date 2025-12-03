#ifndef BLACK_SCHOLES_HPP
#define BLACK_SCHOLES_HPP

#include <string>

double normalCDF(double x);
double normalPDF(double x);

double blackScholesCall(double S, double K, double r, double sigma, double T);
double blackScholesPut(double S, double K, double r, double sigma, double T);

double deltaCall(double S, double K, double r, double sigma, double T);
double deltaPut(double S, double K, double r, double sigma, double T);

double gamma(double S, double K, double r, double sigma, double T);
double vega(double S, double K, double r, double sigma, double T);
double thetaCall(double S, double K, double r, double sigma, double T);
double thetaPut(double S, double K, double r, double sigma, double T);
double rhoCall(double S, double K, double r, double sigma, double T);
double rhoPut(double S, double K, double r, double sigma, double T);

double brownProb(double S, double target, double r, double sigma, double T, const std::string& type);
double vegaNormalized(double mispricing, double vegaVal);
double ivZScore(double sigma, double ivMean, double ivStd);
double gammaRiskScore(double gammaVal, double spot, double movePercent);
double thetaBleedDaily(double thetaVal);
double liquidityScore(double volume, double delta);
double skewEdge(double sigma, double iv_mean, double iv_std, double moneyness);
double smileDistance(double sigma, double iv_mean);

double sabrEnhancedScore(
    double scoreRaw,
    double vegaNorm,
    double ivZ,
    double liquidity,
    double gamRisk,
    double skew,
    double smileDist
);

double dynamicMaxDelta(double T, double moneyness);

#endif
