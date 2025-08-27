#ifndef MATH_H
#define MATH_H

#include <stdint.h>
#include <ctype.h>

#define M_PI 3.14159265358979323846
#define M_E  2.7182818284590452354

#define NAN (*((double*)((uint64_t[]){0x7FF8000000000000ULL})))

double sin(double x);
float sinf(float x);
double cos(double x);
float cosf(float x);
double tan(double x);

double sqrt(double x);
double pow(double x, double y);
double exp(double x);
double frexp(double x, int *exp);
double log(double x);
double log10(double x);

double ceil(double x);
double floor(double x);
double round(double x);

int abs(int x);
double fabs(double x);
double fmod(double x, double y);
double copysign(double x, double y);

double atan(double x);
double atan2(double y, double x);

double fmin(double a, double b);
double fmax(double a, double b);

#endif // MATH_H