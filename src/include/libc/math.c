#include "math.h"

double factorial(int n) {
    double result = 1.0;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

double sin(double x) {
    while (x > M_PI) x -= 2 * M_PI;
    while (x < -M_PI) x += 2 * M_PI;

    double result = 0.0;
    for (int n = 0; n < 10; n++) {
        double term = pow(-1, n) * pow(x, 2 * n + 1) / factorial(2 * n + 1);
        result += term;
    }
    return result;
}

float sinf(float x) {
    while (x > 3.14159265f * 2) x -= 3.14159265f * 2;
    while (x < -3.14159265f * 2) x += 3.14159265f * 2;
    
    float xx = x * x;
    float result = x - x * xx / 6.0f;
    result += x * xx * xx / 120.0f;
    return result;
}

double cos(double x) {
    while (x > M_PI) x -= 2 * M_PI;
    while (x < -M_PI) x += 2 * M_PI;

    double result = 0.0;
    for (int n = 0; n < 10; n++) {
        double term = pow(-1, n) * pow(x, 2 * n) / factorial(2 * n);
        result += term;
    }
    return result;
}

float cosf(float x) {
    while (x > 3.14159265f * 2) x -= 3.14159265f * 2;
    while (x < -3.14159265f * 2) x += 3.14159265f * 2;
    
    float xx = x * x;
    float result = 1.0f - xx/2.0f;
    result += xx * xx / 24.0f;
    result -= xx * xx * xx / 720.0f;
    return result;
}

double tan(double x) {
    return sin(x) / cos(x);
}

double sqrt(double x) {
    if (x < 0) return 0.0 / 0.0;
    if (x == 0) return 0.0;

    double guess = x;
    for (int i = 0; i < 20; i++) {
        guess = 0.5 * (guess + x / guess);
    }
    return guess;
}

double pow(double x, double y) {
    if (y == 0) return 1.0;
    if (x == 0) return 0.0;

    if (y == (int)y) {
        double result = 1.0;
        for (int i = 0; i < (y > 0 ? y : -y); i++) {
            result *= x;
        }
        return y > 0 ? result : 1.0 / result;
    }

    return exp(y * log(x));
}

double exp(double x) {
    double result = 0.0;
    for (int n = 0; n < 20; n++) {
        result += pow(x, n) / factorial(n);
    }
    return result;
}

double log(double x) {
    if (x <= 0) return 0.0 / 0.0;

    double y = x - 1;
    double result = 0.0;
    for (int n = 1; n < 20; n++) {
        if (n % 2 == 1) {
            result += pow(y, n) / n;
        } else {
            result -= pow(y, n) / n;
        }
    }
    return result;
}

double log10(double x) {
    double ln10 = 2.302585092994046;
    double y = 0.0;
    double term = (x - 1.0) / (x + 1.0);
    double term_sq = term * term;
    double pow_term = term;
    double denom = 1.0;
    
    for (int i = 0; i < 10; i++) {
        y += pow_term / denom;
        pow_term *= term_sq;
        denom += 2.0;
    }
    
    return 2.0 * y / ln10;
}

double ceil(double x) {
    int int_part = (int)x;
    if (x > 0 && x != int_part) {
        return int_part + 1;
    }
    return int_part;
}

double floor(double x) {
    int int_part = (int)x;
    if (x < 0 && x != int_part) {
        return int_part - 1;
    }
    return int_part;
}

double round(double x) {
    if (x >= 0) {
        return (int)(x + 0.5);
    } else {
        return (int)(x - 0.5);
    }
}

int abs(int x) {
    return (x < 0) ? -x : x;
}

double fabs(double x) {
    return x < 0 ? -x : x;
}