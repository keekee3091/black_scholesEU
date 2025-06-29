#ifndef BLACK_SCHOLES_HPP	
#define BLACK_SCHOLES_HPP
#include <string>

double normalCDF(double x);
double blackScholesCall(double S, double K, double r, double sigma, double T);
double blackScholesPut(double S, double K, double r, double sigma, double T);
double brownProb(double S, double target, double r, double sigma, double T, const std::string& type);

double deltaCall(double S, double K, double r, double sigma, double T);
double deltaPut(double S, double K, double r, double sigma, double T);
double gamma(double S, double K, double r, double sigma, double T);

#endif