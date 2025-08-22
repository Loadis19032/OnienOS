#include "assert.h"
#include <stdio.h>

void __assert_fail(const char *expr, const char *file, int line, const char *func) {
    printf("%s:%d: %s: Assertion `%s` failed.\n", file, line, func, expr);

    return;
}