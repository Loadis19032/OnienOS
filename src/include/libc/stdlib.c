#include "stdlib.h"
#include <ctype.h>
#include <math.h>
#include "../../kernel/mm/mem.h"

void* malloc(size_t size) {
    return kmalloc(size);
}

void* realloc(void* ptr, size_t size) {
    return krealloc(ptr, size);
}

void* calloc(size_t num, size_t size) {
    return kcalloc(num, size);
}

void free(void* ptr) {
    return kfree(ptr);
}

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

static void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char* itoa(int num, char* str) {
    int i = 0;
    int is_negative = 0;

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    do {
        str[i++] = num % 10 + '0';
        num /= 10;                
    } while (num > 0);

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    reverse(str, i);

    return str;
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

static void swap(void *a, void *b, size_t size) {
    char *temp = malloc(size);
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
    free(temp);
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    if (nmemb <= 1) return;
    
    char *array = (char*)base;
    
    char *mid = array + (nmemb / 2) * size;
    char *end = array + (nmemb - 1) * size;
    
    if (compar(array, mid) > 0) swap(array, mid, size);
    if (compar(mid, end) > 0) swap(mid, end, size);
    if (compar(array, mid) > 0) swap(array, mid, size);
    
    char *pivot = mid;
    char *i = array - size;
    
    for (char *j = array; j <= end; j += size) {
        if (compar(j, pivot) <= 0) {
            i += size;
            swap(i, j, size);
        }
    }
    
    size_t pivot_index = (i - array) / size;
    size_t left_count = pivot_index;
    size_t right_count = nmemb - pivot_index - 1;
    
    qsort(array, left_count, size, compar);
    qsort(i + size, right_count, size, compar);
}