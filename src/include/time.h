#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stddef.h>

typedef int64_t time_t;

typedef struct {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
} tm_t;

time_t time(time_t *timer);
tm_t *localtime(const time_t *timer);
char *asctime(const tm_t *timeptr);
char *ctime(const time_t *timer);
double difftime(time_t time1, time_t time0);
time_t mktime(tm_t *timeptr);
size_t strftime(char *str, size_t maxsize, const char *format, const tm_t *timeptr);
tm_t *gmtime(const time_t *timer);

inline uint64_t sleep(uint64_t milliseconds) {
    Sleep(milliseconds);
}

#endif