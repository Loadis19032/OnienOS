#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_BLACK   0x00
#define VGA_BLUE    0x01
#define VGA_GREEN   0x02
#define VGA_CYAN    0x03
#define VGA_RED     0x04
#define VGA_MAGENTA 0x05
#define VGA_BROWN   0x06
#define VGA_LGRAY   0x07
#define VGA_DGRAY   0x08
#define VGA_LBLUE   0x09
#define VGA_LGREEN  0x0A
#define VGA_LCYAN   0x0B
#define VGA_LRED    0x0C
#define VGA_LMAGENTA 0x0D
#define VGA_YELLOW  0x0E
#define VGA_WHITE   0x0F

#define VGA_COLOR(bg, fg) (((bg & 0x0F) << 4) | (fg & 0x0F))

int putchar(int c);
void clear();

int vsprintf(char* buffer, const char* format, va_list args);
int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
int sprintf(char* buffer, const char* format, ...);
int snprintf(char* buffer, size_t size, const char* format, ...);
int printf(const char* fmt, ...);

void setcursor(uint8_t x, uint8_t y);
void setcolor(int color);

#endif // STDIO_H