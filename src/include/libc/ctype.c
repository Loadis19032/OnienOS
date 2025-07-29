#include "ctype.h"

int isalpha(int c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || 
            c == '\r' || c == '\v' || c == '\f');
}

int isupper(int c) {
    return ((unsigned)c - 'A') <= ('Z' - 'A');
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}

int isnan(double x) {
    uint64_t bits;
    memcpy(&bits, &x, sizeof(double));
    return ((bits >> 52) & 0x7FF) == 0x7FF && (bits & 0xFFFFFFFFFFFFF) != 0;
}

int isinf(double x) {
    uint64_t bits;
    memcpy(&bits, &x, sizeof(double));
    return ((bits >> 52) & 0x7FF) == 0x7FF && (bits & 0xFFFFFFFFFFFFF) == 0;
}