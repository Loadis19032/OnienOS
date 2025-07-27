#include "stdlib.h"
#include <ctype.h>
#include <math.h>

int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    while (isspace(*str)) {
        str++;
    }

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (isdigit(*str)) {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

double atof(const char *str) {
    double result = 0.0;
    double fraction = 1.0;
    int sign = 1;

    while (isspace(*str)) {
        str++;
    }

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (isdigit(*str)) {
        result = result * 10.0 + (*str - '0');
        str++;
    }

    if (*str == '.') {
        str++;
        while (isdigit(*str)) {
            fraction *= 0.1;
            result += (*str - '0') * fraction;
            str++;
        }
    }

    if (*str == 'e' || *str == 'E') {
        str++;
        int exp_sign = 1;
        int exponent = 0;

        if (*str == '-') {
            exp_sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }

        while (isdigit(*str)) {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        result *= pow(10.0, exp_sign * exponent);
    }

    return sign * result;
}

long int strtol(const char *str, char **endptr, int base) {
    long int result = 0;
    int sign = 1;

    while (isspace(*str)) {
        str++;
    }

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    if (base == 0) {
        if (*str == '0') {
            str++;
            if (*str == 'x' || *str == 'X') {
                base = 16;
                str++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    while (1) {
        int digit;
        if (isdigit(*str)) {
            digit = *str - '0';
        } else if (isalpha(*str)) {
            digit = toupper(*str) - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        str++;
    }

    if (endptr != NULL) {
        *endptr = (char *)str;
    }

    return sign * result;
}

static unsigned long int next = 1;

int rand(void) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % (RAND_MAX + 1);
}

void srand(unsigned int seed) {
    next = seed;
}