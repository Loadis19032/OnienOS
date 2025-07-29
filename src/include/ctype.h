#ifndef CTYPE_H
#define CTYPE_H

#include <string.h>
#include <stdint.h>

int isalpha(int c);
int isdigit(int c);
int isspace(int c);
int isupper(int c);

int tolower(int c);
int toupper(int c);

int isnan(double x);
int isinf(double x);

#endif // CTYPE_H