#ifndef _ASSERT_H
#define _ASSERT_H

#include <stdio.h>
#include <stdlib.h>

// Отключаем предыдущее определение (если было)
#undef assert

#ifdef NDEBUG
    #define assert(condition) ((void)0)  // Режим без отладки — assert ничего не делает
#else
    // Объявляем функцию, которая будет реализована в assert.c
    void __assert_fail(const char *expr, const char *file, int line, const char *func);
    
    // Макрос, который вызывает __assert_fail при ошибке
    #define assert(condition) \
        ((void)((condition) || (__assert_fail(#condition, __FILE__, __LINE__, __func__), 0)))
#endif

#endif /* _ASSERT_H */