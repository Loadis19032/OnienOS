#include "string.h"
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>

static int is_delim(char c, const char* delim) {
    while (*delim) {
        if (*delim == c) return 1;
        delim++;
    }
    return 0;
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        char* lastd = d + (n - 1);
        const char* lasts = s + (n - 1);
        while (n--) *lastd-- = *lasts--;
    }
    return dest;
}

char* strcpy(char* dest, const char* src) {
    char* ptr = dest;
    while ((*ptr++ = *src++) != '\0');
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* ptr = dest;
    while (n-- && (*ptr++ = *src++) != '\0');
    while (n-- > 0) *ptr++ = '\0';
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest + strlen(dest);
    while ((*ptr++ = *src++) != '\0');
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    char* ptr = dest + strlen(dest);
    while (n-- && (*ptr++ = *src++) != '\0');
    *ptr = '\0';
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = s1;
    const unsigned char* p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return n == 0 ? 0 : *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strcoll(const char* s1, const char* s2) {
    return strcmp(s1, s2);
}

size_t strxfrm(char* dest, const char* src, size_t n) {
    size_t len = strlen(src);
    if (n > 0) {
        strncpy(dest, src, n);
        dest[n-1] = '\0';
    }
    return len;
}

void* memchr(const void* s, int c, size_t n) {
    const unsigned char* p = s;
    while (n--) {
        if (*p == (unsigned char)c) return (void*)p;
        p++;
    }
    return NULL;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (*s == '\0') return NULL;
        s++;
    }
    return (char*)s;
}

char* strrchr(const char* s, int c) {
    const char* last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    return (char*)((char)c == '\0' ? s : last);
}

size_t strcspn(const char* s, const char* reject) {
    size_t count = 0;
    while (*s && !strchr(reject, *s)) {
        s++;
        count++;
    }
    return count;
}

size_t strspn(const char* s, const char* accept) {
    size_t count = 0;
    while (*s && strchr(accept, *s)) {
        s++;
        count++;
    }
    return count;
}

char* strpbrk(const char* s, const char* accept) {
    while (*s) {
        if (strchr(accept, *s)) return (char*)s;
        s++;
    }
    return NULL;
}

char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0') return (char*)haystack;
    
    for (const char* h = haystack; *h != '\0'; h++) {
        const char* n = needle;
        const char* h_ptr = h;
        while (*n != '\0' && *h_ptr == *n) {
            h_ptr++;
            n++;
        }
        if (*n == '\0') return (char*)h;
    }
    return NULL;
}

char* strtok(char* str, const char* delim) {
    static char* saved_ptr = NULL;
    
    if (str) {
        saved_ptr = str;
    } else if (!saved_ptr) {
        return NULL;
    }
    
    str = saved_ptr;
    while (*str && is_delim(*str, delim)) {
        str++;
    }
    
    if (*str == '\0') {
        saved_ptr = NULL;
        return NULL;
    }
    
    char* token = str;
    while (*str && !is_delim(*str, delim)) {
        str++;
    }
    
    if (*str == '\0') {
        saved_ptr = NULL;
    } else {
        *str = '\0';
        saved_ptr = str + 1;
    }
    
    return token;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    if (!str && !(str = *saveptr)) {
        return NULL;
    }

    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    char *end = str + strcspn(str, delim);
    if (*end == '\0') {
        *saveptr = NULL;
        return str;
    }

    *end = '\0';
    *saveptr = end + 1;
    return str;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

char* strdup(const char* s) {
    if (s == NULL)
        return NULL;
    
    size_t len = strlen(s) + 1;
    char* dup = malloc(len);
    
    if (dup != NULL)
        memcpy(dup, s, len);
    
    return dup;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) {
            return diff;
        }
        s1++;
        s2++;
    }
    
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static const char* error_messages[] = {
    "No error",
    "Operation not permitted",
    "No such file or directory",
};

char* strerror(int errnum) {
    if (errnum < 0 || errnum >= (int)(sizeof(error_messages)/sizeof(error_messages[0]))) {
        return "Unknown error";
    }
    return (char*)error_messages[errnum];
}