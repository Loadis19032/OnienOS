#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

int atoi(const char *str);
double atof(const char *str);
long int strtol(const char *str, char **endptr, int base);

int rand(void);
void srand(unsigned int seed);

#define RAND_MAX 32767

#endif // STDLIB_H