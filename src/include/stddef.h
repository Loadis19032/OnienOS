#ifndef _STDDEF_H
#define _STDDEF_H

/* 7.17 Common definitions */

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long
#endif
typedef __SIZE_TYPE__ size_t;

#ifndef __PTRDIFF_TYPE__
#define __PTRDIFF_TYPE__ long
#endif
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef __WCHAR_TYPE__
#define __WCHAR_TYPE__ int
#endif
typedef __WCHAR_TYPE__ wchar_t;

#ifndef offsetof
#define offsetof(type, member) ((size_t)&((type*)0)->member)
#endif

#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN INTPTR_MIN
#endif

#ifndef PTRDIFF_MAX
#define PTRDIFF_MAX INTPTR_MAX
#endif

#ifndef SIZE_MAX
#define SIZE_MAX UINTPTR_MAX
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#endif /* _STDDEF_H */