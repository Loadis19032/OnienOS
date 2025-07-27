#include "math.h"

static double factorial(int n) {
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

double fabs(double x) {
    return x < 0 ? -x : x;
}