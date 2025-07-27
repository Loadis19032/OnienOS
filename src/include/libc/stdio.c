#include "stdio.h"
#include <stdarg.h>

#define VIDEO_MEM 0xB8000
#define WIDTH 80
#define HEIGHT 25

static int cursor_x = 0;
static int cursor_y = 0;
static int current_color = VGA_COLOR(VGA_BLACK, VGA_LGRAY);

void setcolor(int color) {
    current_color = color;
}

void clear() {
    volatile char* vidmem = (volatile char*)VIDEO_MEM;
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int pos = (y * WIDTH + x) * 2;
            vidmem[pos] = ' ';
            vidmem[pos+1] = current_color;
        }
    }
    
    cursor_x = 0;
    cursor_y = 0;
}

int putchar(int c) {
    volatile char* vidmem = (volatile char*)VIDEO_MEM;
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else if (c >= ' ') {
        int pos = (cursor_y * WIDTH + cursor_x) * 2;
        vidmem[pos] = c;
        vidmem[pos+1] = current_color;
        cursor_x++;
    }
    
    if (cursor_x >= WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= HEIGHT) {
        for (int y = 1; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int src = (y * WIDTH + x) * 2;
                int dst = ((y-1) * WIDTH + x) * 2;
                vidmem[dst] = vidmem[src];
                vidmem[dst+1] = vidmem[src+1];
            }
        }
        
        for (int x = 0; x < WIDTH; x++) {
            int pos = ((HEIGHT-1) * WIDTH + x) * 2;
            vidmem[pos] = ' ';
            vidmem[pos+1] = current_color;
        }
        
        cursor_y = HEIGHT - 1;
    }
    
    return c;
}

static void print_num(unsigned long num, int base) {
    const char* digits = "0123456789ABCDEF";
    char buf[64];
    int pos = 0;
    
    if (num == 0) {
        putchar('0');
        return;
    }
    
    while (num > 0) {
        buf[pos++] = digits[num % base];
        num /= base;
    }
    
    while (--pos >= 0) {
        putchar(buf[pos]);
    }
}

int printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    int count = 0;
    char c;
    
    while ((c = *fmt++)) {
        if (c != '%') {
            putchar(c);
            count++;
            continue;
        }
        
        c = *fmt++;
        switch (c) {
            case 'd':
            case 'i': {
                long long num = va_arg(args, long long);
                if (num < 0) {
                    putchar('-');
                    num = -num;
                }
                print_num(num, 10);
                break;
            }
            case 'u': {
                unsigned long long num = va_arg(args, unsigned long long);
                print_num(num, 10);
                break;
            }
            case 'x':
            case 'X': {
                unsigned long long num = va_arg(args, unsigned long long);
                print_num(num, 16);
                break;
            }
            case 'o': {
                unsigned long long num = va_arg(args, unsigned long long);
                print_num(num, 8);
                break;
            }
            case 'c': {
                int ch = va_arg(args, int);
                putchar(ch);
                count++;
                break;
            }
            case 's': {
                const char* str = va_arg(args, const char*);
                while (*str) {
                    putchar(*str++);
                    count++;
                }
                break;
            }
            case 'p': {
                void* ptr = va_arg(args, void*);
                putchar('0');
                putchar('x');
                print_num((unsigned long long)ptr, 16);
                break;
            }
            case '%': {
                putchar('%');
                count++;
                break;
            }
            case 'l': {
                // Handle long/long long specifiers
                char next = *fmt++;
                if (next == 'l') {
                    // %ll - long long
                    next = *fmt++;
                    switch (next) {
                        case 'd':
                        case 'i': {
                            long long num = va_arg(args, long long);
                            if (num < 0) {
                                putchar('-');
                                num = -num;
                            }
                            print_num(num, 10);
                            break;
                        }
                        case 'u': {
                            unsigned long long num = va_arg(args, unsigned long long);
                            print_num(num, 10);
                            break;
                        }
                        case 'x':
                        case 'X': {
                            unsigned long long num = va_arg(args, unsigned long long);
                            print_num(num, 16);
                            break;
                        }
                        default:
                            putchar('%');
                            putchar('l');
                            putchar('l');
                            putchar(next);
                            count += 4;
                            break;
                    }
                } else {
                    // %l - long
                    switch (next) {
                        case 'd':
                        case 'i': {
                            long num = va_arg(args, long);
                            if (num < 0) {
                                putchar('-');
                                num = -num;
                            }
                            print_num(num, 10);
                            break;
                        }
                        case 'u': {
                            unsigned long num = va_arg(args, unsigned long);
                            print_num(num, 10);
                            break;
                        }
                        case 'x':
                        case 'X': {
                            unsigned long num = va_arg(args, unsigned long);
                            print_num(num, 16);
                            break;
                        }
                        default:
                            putchar('%');
                            putchar('l');
                            putchar(next);
                            count += 3;
                            break;
                    }
                }
                break;
            }
            default: {
                putchar('%');
                putchar(c);
                count += 2;
                break;
            }
        }
    }
    
    va_end(args);
    return count;
}