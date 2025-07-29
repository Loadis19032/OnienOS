/* float.h - Characteristics of floating-point types (x86_64 specific) */
#ifndef _FLOAT_H
#define _FLOAT_H

/* Общий базис (основание) для экспоненты (всегда 2 на x86_64) */
#define FLT_RADIX       2

/*---------------------------------------
 * Характеристики типа `float` (32-bit IEEE-754)
 *---------------------------------------*/
#define FLT_MANT_DIG    24      /* Количество бит мантиссы (p) */
#define FLT_DIG         6       /* Десятичных цифр точности */
#define FLT_MIN_EXP     (-125)  /* Минимальная экспонента (e_min) */
#define FLT_MAX_EXP     128     /* Максимальная экспонента (e_max) */
#define FLT_MIN_10_EXP  (-37)   /* Минимальная десятичная экспонента (~ log10(2^e_min)) */
#define FLT_MAX_10_EXP  38      /* Максимальная десятичная экспонента (~ log10(2^e_max)) */

/* Значения */
#define FLT_MAX         3.402823466e+38F        /* (1 - 2^-p) * 2^e_max */
#define FLT_EPSILON     1.192092896e-07F        /* 2^(1 - p) */
#define FLT_MIN         1.175494351e-38F        /* 2^(e_min - 1) */

/*---------------------------------------
 * Характеристики типа `double` (64-bit IEEE-754)
 *---------------------------------------*/
#define DBL_MANT_DIG    53
#define DBL_DIG         15
#define DBL_MIN_EXP     (-1021)
#define DBL_MAX_EXP     1024
#define DBL_MIN_10_EXP  (-307)
#define DBL_MAX_10_EXP  308

/* Значения */
#define DBL_MAX         1.7976931348623157e+308 /* (1 - 2^-p) * 2^e_max */
#define DBL_EPSILON     2.2204460492503131e-16  /* 2^(1 - p) */
#define DBL_MIN         2.2250738585072014e-308 /* 2^(e_min - 1) */

/*---------------------------------------
 * Характеристики типа `long double` (80-bit x87 Extended Precision)
 *---------------------------------------*/
#define LDBL_MANT_DIG   64      /* 64 бита мантиссы (1 бит знака + 63 явных) */
#define LDBL_DIG        18      /* Десятичных цифр точности */
#define LDBL_MIN_EXP    (-16381) /* Минимальная экспонента */
#define LDBL_MAX_EXP    16384    /* Максимальная экспонента */
#define LDBL_MIN_10_EXP (-4931)  /* Минимальная десятичная экспонента */
#define LDBL_MAX_10_EXP 4932     /* Максимальная десятичная экспонента */

/* Значения */
#define LDBL_MAX        1.18973149535723176502e+4932L  /* (1 - 2^-p) * 2^e_max */
#define LDBL_EPSILON    1.08420217248550443401e-19L   /* 2^(1 - p) */
#define LDBL_MIN        3.36210314311209350626e-4932L /* 2^(e_min - 1) */

/*---------------------------------------
 * Дополнительные макросы (C99)
 *---------------------------------------*/
#define DECIMAL_DIG     21  /* Максимальное количество десятичных цифр, */
                            /* которых хватит для round-trip любого long double */

#endif /* _FLOAT_H */