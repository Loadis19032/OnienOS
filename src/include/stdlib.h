#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void* calloc(size_t num, size_t size);
void free(void* ptr);

int atoi(const char *str);
double atof(const char *str);
char* itoa(int num, char* str);
long int strtol(const char *str, char **endptr, int base);

int rand(void);
void srand(unsigned int seed);

#define RAND_MAX 32767

#endif // STDLIB_H