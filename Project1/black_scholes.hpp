#ifndef BLACK_SCHOLES_HPP	
#define BLACK_SCHOLES_HPP

double normalCDF(double x);
double blackScholesCall(double S, double K, double r, double sigma, double T);
double blackScholesPut(double S, double K, double r, double sigma, double T);

double deltaCall(double S, double K, double r, double sigma, double T);
double deltaPut(double S, double K, double r, double sigma, double T);
double gamma(double S, double K, double r, double sigma, double T);

#endif