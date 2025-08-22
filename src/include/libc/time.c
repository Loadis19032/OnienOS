#include "time.h"
#include <string.h>

#define CMOS_ADDRESS  0x70
#define CMOS_DATA     0x71

#define RTC_SECOND    0x00
#define RTC_MINUTE    0x02
#define RTC_HOUR      0x04
#define RTC_DAY       0x07
#define RTC_MONTH     0x08
#define RTC_YEAR      0x09
#define RTC_STATUS_A  0x0A
#define RTC_STATUS_B  0x0B

static uint8_t cmos_read(uint8_t offset) {
    __asm__ volatile("outb %0, %1" : : "a"(offset), "Nd"(CMOS_ADDRESS));
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(CMOS_DATA));
    return value;
}

static uint8_t rtc_updating() {
    return cmos_read(RTC_STATUS_A) & 0x80;
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

static tm_t read_rtc() {
    tm_t time = {0};
    uint8_t reg_b;

    while (rtc_updating());

    time.tm_sec = cmos_read(RTC_SECOND);
    time.tm_min = cmos_read(RTC_MINUTE);
    time.tm_hour = cmos_read(RTC_HOUR);
    time.tm_mday = cmos_read(RTC_DAY);
    time.tm_mon = cmos_read(RTC_MONTH) - 1;
    time.tm_year = cmos_read(RTC_YEAR);
    reg_b = cmos_read(RTC_STATUS_B);

    if (!(reg_b & 0x04)) {
        time.tm_sec = bcd_to_bin(time.tm_sec);
        time.tm_min = bcd_to_bin(time.tm_min);
        time.tm_hour = bcd_to_bin(time.tm_hour);
        time.tm_mday = bcd_to_bin(time.tm_mday);
        time.tm_mon = bcd_to_bin(time.tm_mon) - 1;
        time.tm_year = bcd_to_bin(time.tm_year);
    }

    if (!(reg_b & 0x02) && (time.tm_hour & 0x80)) {
        time.tm_hour = ((time.tm_hour & 0x7F) + 12) % 24;
    }

    time.tm_year += 100;

    int y = time.tm_year + 1900;
    int m = time.tm_mon + 1;
    int d = time.tm_mday;

    if (m < 3) {
        m += 12;
        y--;
    }
    time.tm_wday = (d + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;

    return time;
}

time_t time(time_t *timer) {
    tm_t now = read_rtc();
    time_t secs = (now.tm_year - 70) * 31536000 +
                  now.tm_mday * 86400 +
                  now.tm_hour * 3600 +
                  now.tm_min * 60 +
                  now.tm_sec;
    if (timer) *timer = secs;
    return secs;
}

static const int days_in_months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

tm_t *localtime(const time_t *timer) {
    static tm_t result;
    if (!timer) {
        result = read_rtc();
        return &result;
    }

    time_t t = *timer;
    result.tm_sec = t % 60; t /= 60;
    result.tm_min = t % 60; t /= 60;
    result.tm_hour = t % 24; t /= 24;
    
    result.tm_year = 70;
    while (1) {
        int days_in_year = is_leap_year(1900 + result.tm_year) ? 366 : 365;
        if (t < days_in_year) break;
        t -= days_in_year;
        result.tm_year++;
    }
    
    result.tm_mon = 0;
    while (1) {
        int days_in_month = days_in_months[result.tm_mon];
        if (result.tm_mon == 1 && is_leap_year(1900 + result.tm_year)) {
            days_in_month = 29;
        }
        if (t < days_in_month) break;
        t -= days_in_month;
        result.tm_mon++;
    }
    
    result.tm_mday = t + 1;
    
    int y = 1900 + result.tm_year;
    int m = result.tm_mon + 1;
    int d = result.tm_mday;
    
    if (m < 3) {
        m += 12;
        y--;
    }
    result.tm_wday = (d + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    
    return &result;
}

static char itoa(int num) {
    return num + '0';
}

char *asctime(const tm_t *timeptr) {
    static const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static char buf[26];
    char *p = buf;

    memcpy(p, wday[timeptr->tm_wday], 3);
    p += 3;
    *p++ = ' ';

    memcpy(p, mon[timeptr->tm_mon], 3);
    p += 3;
    *p++ = ' ';

    *p++ = (timeptr->tm_mday / 10) ? itoa(timeptr->tm_mday / 10) : ' ';
    *p++ = itoa(timeptr->tm_mday % 10);
    *p++ = ' ';

    *p++ = itoa(timeptr->tm_hour / 10);
    *p++ = itoa(timeptr->tm_hour % 10);
    *p++ = ':';

    *p++ = itoa(timeptr->tm_min / 10);
    *p++ = itoa(timeptr->tm_min % 10);
    *p++ = ':';

    *p++ = itoa(timeptr->tm_sec / 10);
    *p++ = itoa(timeptr->tm_sec % 10);
    *p++ = ' ';

    int year = 1900 + timeptr->tm_year;
    *p++ = itoa(year / 1000);
    *p++ = itoa((year / 100) % 10);
    *p++ = itoa((year / 10) % 10);
    *p++ = itoa(year % 10);
    *p++ = '\n';
    *p = '\0';

    return buf;
}

char *ctime(const time_t *timer) {
    return asctime(localtime(timer));
}

double difftime(time_t time1, time_t time0) {
    return (double)(time1 - time0);
}

time_t mktime(tm_t *timeptr) {
    time_t result;
    result = timeptr->tm_sec + timeptr->tm_min*60 + timeptr->tm_hour*3600 +
             timeptr->tm_mday*86400 + timeptr->tm_mon*2678400 +
             (timeptr->tm_year-70)*31536000;
    return result;
}

size_t strftime(char *str, size_t maxsize, const char *format, const tm_t *timeptr) {
    if (!str || !timeptr || !format || maxsize == 0) {
        return 0;
    }

    size_t pos = 0;
    const char *fmt = format;

    while (*fmt && pos < maxsize - 1) {
        if (*fmt != '%') {
            str[pos++] = *fmt++;
            continue;
        }

        fmt++;
        if (!*fmt) break;

        switch (*fmt) {
            case 'Y': {
                int year = 1900 + timeptr->tm_year;
                if (pos + 4 >= maxsize) return 0;
                str[pos++] = '0' + (year / 1000);
                str[pos++] = '0' + ((year / 100) % 10);
                str[pos++] = '0' + ((year / 10) % 10);
                str[pos++] = '0' + (year % 10);
                break;
            }
            case 'm': {
                int mon = timeptr->tm_mon + 1;
                if (pos + 2 >= maxsize) return 0;
                str[pos++] = '0' + (mon / 10);
                str[pos++] = '0' + (mon % 10);
                break;
            }
            case 'd': {
                if (pos + 2 >= maxsize) return 0;
                str[pos++] = '0' + (timeptr->tm_mday / 10);
                str[pos++] = '0' + (timeptr->tm_mday % 10);
                break;
            }
            case 'H': {
                if (pos + 2 >= maxsize) return 0;
                str[pos++] = '0' + (timeptr->tm_hour / 10);
                str[pos++] = '0' + (timeptr->tm_hour % 10);
                break;
            }
            case 'M': {
                if (pos + 2 >= maxsize) return 0;
                str[pos++] = '0' + (timeptr->tm_min / 10);
                str[pos++] = '0' + (timeptr->tm_min % 10);
                break;
            }
            case 'S': {
                if (pos + 2 >= maxsize) return 0;
                str[pos++] = '0' + (timeptr->tm_sec / 10);
                str[pos++] = '0' + (timeptr->tm_sec % 10);
                break;
            }
            case '%': {
                if (pos + 1 >= maxsize) return 0;
                str[pos++] = '%';
                break;
            }
            default: {
                if (pos + 2 >= maxsize) return 0;
                str[pos++] = '%';
                str[pos++] = *fmt;
                break;
            }
        }
        fmt++;
    }

    str[pos] = '\0';
    return pos;
}

tm_t *gmtime(const time_t *timer) {
    return localtime(timer);
}