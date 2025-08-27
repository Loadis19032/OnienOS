#include "stdio.h"
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdbool.h>
#include <asm/io.h>

#define VIDEO_MEM 0xB8000
#define WIDTH 80
#define HEIGHT 25

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW 0x0F

#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW 0x0F

void setcursor(uint8_t x, uint8_t y) {
    if (x >= 80) x = 79;
    if (y >= 25) y = 24;
    
    uint16_t position = y * 80 + x;
    
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOW);
    outb(VGA_DATA_REGISTER, (uint8_t)(position & 0xFF));
    
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_HIGH);
    outb(VGA_DATA_REGISTER, (uint8_t)((position >> 8) & 0xFF));
}

#define VIDEO_MEM 0xB8000
#define WIDTH 80
#define HEIGHT 25
#define VGA_SIZE (WIDTH * HEIGHT)

static volatile uint16_t* vga_buffer = (volatile uint16_t*)VIDEO_MEM;

static void scroll(void) {
    for (int y = 0; y < HEIGHT-1; y++) {
        for (int x = 0; x < WIDTH; x++) {
            vga_buffer[y * WIDTH + x] = vga_buffer[(y + 1) * WIDTH + x];
        }
    }

    uint16_t blank = (' ' << 8) | current_color;
    for (int x = 0; x < WIDTH; x++) {
        vga_buffer[(HEIGHT-1) * WIDTH + x] = blank;
    }
}

#define STATE_NORMAL 0
#define STATE_ESC    1
#define STATE_CSI    2

static int ansi_state = STATE_NORMAL;
static int ansi_params[16];
static int ansi_count = 0;

static void handle_ansi_command(char c) {
    switch (c) {
        case 'm':
            for (int i = 0; i <= ansi_count; i++) {
                int param = ansi_params[i];
                if (param == 0) {
                    current_color = VGA_COLOR(VGA_BLACK, VGA_LGRAY);
                } else if (param >= 30 && param <= 37) {
                    static const uint8_t ansi_to_vga[] = {VGA_BLACK, VGA_RED, VGA_GREEN, VGA_BROWN, VGA_BLUE, VGA_MAGENTA, VGA_CYAN, VGA_LGRAY};
                    current_color = (current_color & 0xF0) | ansi_to_vga[param - 30];
                } else if (param >= 40 && param <= 47) {
                    static const uint8_t ansi_to_vga[] = {VGA_BLACK, VGA_RED, VGA_GREEN, VGA_BROWN, VGA_BLUE, VGA_MAGENTA, VGA_CYAN, VGA_LGRAY};
                    current_color = (current_color & 0x0F) | (ansi_to_vga[param - 40] << 4);
                }
            }
            break;
        case 'J':
            if (ansi_params[0] == 2) clear();
            break;
        case 'K':
            if (ansi_params[0] == 0) {
                for (int x = cursor_x; x < WIDTH; x++)
                    vga_buffer[cursor_y * WIDTH + x] = (current_color << 8) | ' ';
            }
            break;
        case 'H':
            cursor_x = ansi_params[1] ? ansi_params[1] - 1 : 0;
            cursor_y = ansi_params[0] ? ansi_params[0] - 1 : 0;
            cursor_x = MIN(WIDTH-1, MAX(0, cursor_x));
            cursor_y = MIN(HEIGHT-1, MAX(0, cursor_y));
            setcursor(cursor_x, cursor_y);
            break;
    }
}

int putchar(int c) {
    if (cursor_x >= 80 || cursor_y >= 25) return 0;

    if (ansi_state == STATE_NORMAL) {
        if (c == '\033') {
            ansi_state = STATE_ESC;
            return 0;
        }

        if (c == '\b') {
            if (cursor_x > 0) cursor_x--;
            else if (cursor_y > 0) { cursor_y--; cursor_x = WIDTH-1; }
            vga_buffer[cursor_y * WIDTH + cursor_x] = (current_color << 8) | ' ';
            setcursor(cursor_x, cursor_y);
            return c;
        }
        if (c == '\n') {
            cursor_x = 0;
            if (++cursor_y >= HEIGHT) {
                scroll();
                cursor_y = HEIGHT - 1;
            }
            setcursor(cursor_x, cursor_y);
            return c;
        }
        if (c == '\t') {
            for (int i = 1; i < 4; i++) {
                cursor_x++;
            }
            setcursor(cursor_x, cursor_y);
        }
        if (c >= ' ') {
            vga_buffer[cursor_y * WIDTH + cursor_x] = (current_color << 8) | c;
            if (++cursor_x >= WIDTH) {
                cursor_x = 0;
                if (++cursor_y >= HEIGHT) {
                    scroll();
                    cursor_y = HEIGHT - 1;
                }
            }
            setcursor(cursor_x, cursor_y);
        }
        return c;
    } else if (ansi_state == STATE_ESC) {
        ansi_state = (c == '[') ? STATE_CSI : STATE_NORMAL;
        return 0;
    } else if (ansi_state == STATE_CSI) {
        if (c >= '0' && c <= '9') {
            if (ansi_count < 16) {
                ansi_params[ansi_count] = ansi_params[ansi_count] * 10 + (c - '0');
            }
        } else if (c == ';') {
            if (ansi_count < 15) ansi_count++;
        } else {
            handle_ansi_command(c);
            ansi_state = STATE_NORMAL;
            ansi_count = 0;
            for (int i = 0; i < 16; i++) ansi_params[i] = 0;
        }
        return 0;
    }
    return c;
}

void move_cursor(int8_t dx, int8_t dy) {
    int16_t new_x = cursor_x + dx;
    int16_t new_y = cursor_y + dy;
    
    if (new_x < 0) new_x = 0;
    if (new_x >= WIDTH) new_x = WIDTH - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= HEIGHT) new_y = HEIGHT - 1;
    
    cursor_x = new_x;
    cursor_y = new_y;
    setcursor(cursor_x, cursor_y);
}

typedef struct {
    bool left_align;   
    bool show_sign;    
    bool space_sign;   
    bool alt_form;     
    bool zero_pad;     
    int width;         
    int precision;     
    bool has_precision;
} format_flags_t;

static void print_char(char c) {
    putchar(c);
}

static void print_string(const char *str) {
    while (*str) {
        print_char(*str++);
    }
}

static void print_repeat(char c, int count) {
    while (count-- > 0) {
        print_char(c);
    }
}

static int num_to_str(unsigned long long num, char *buf, int base, bool uppercase) {
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;
    
    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            buf[i++] = digits[num % base];
            num /= base;
        }
    }
    
    int len = i;
    for (int j = 0; j < len / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[len - 1 - j];
        buf[len - 1 - j] = temp;
    }
    
    return len;
}

static const char* parse_flags(const char *fmt, format_flags_t *flags) {
    memset(flags, 0, sizeof(format_flags_t));
    flags->precision = -1;

    while (*fmt) {
        switch (*fmt) {
            case '-': flags->left_align = true; fmt++; break;
            case '+': flags->show_sign = true; fmt++; break;
            case ' ': flags->space_sign = true; fmt++; break;
            case '#': flags->alt_form = true; fmt++; break;
            case '0': flags->zero_pad = true; fmt++; break;
            default: goto parse_width;
        }
    }
    
parse_width:
    if (*fmt >= '1' && *fmt <= '9') {
        flags->width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            flags->width = flags->width * 10 + (*fmt - '0');
            fmt++;
        }
    }
    
    if (*fmt == '.') {
        fmt++;
        flags->has_precision = true;
        flags->precision = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            flags->precision = flags->precision * 10 + (*fmt - '0');
            fmt++;
        }
    }
    
    return fmt;
}

static void format_string(const char *str, format_flags_t *flags) {
    if (!str) str = "(null)";
    
    int len = strlen(str);
    
    if (flags->has_precision && flags->precision < len) {
        len = flags->precision;
    }
    
    int padding = flags->width > len ? flags->width - len : 0;
    
    if (!flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
    
    for (int i = 0; i < len; i++) {
        print_char(str[i]);
    }
    
    if (flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
}

static void format_char(char c, format_flags_t *flags) {
    int padding = flags->width > 1 ? flags->width - 1 : 0;
    
    if (!flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
    
    print_char(c);
    
    if (flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
}

static void format_number(long long num, format_flags_t *flags, int base, bool uppercase, bool is_signed) {
    char buf[64];
    bool negative = false;
    unsigned long long abs_num;
    
    if (is_signed && num < 0) {
        negative = true;
        abs_num = -num;
    } else {
        abs_num = num;
    }
    
    int num_len = num_to_str(abs_num, buf, base, uppercase);
    
    int precision = flags->has_precision ? flags->precision : 1;
    if (flags->has_precision && flags->precision == 0 && abs_num == 0) {
        num_len = 0;
        buf[0] = '\0';
    }
    
    int zeros_needed = precision > num_len ? precision - num_len : 0;
    
    char prefix[3] = {0};
    int prefix_len = 0;
    
    if (negative) {
        prefix[prefix_len++] = '-';
    } else if (flags->show_sign) {
        prefix[prefix_len++] = '+';
    } else if (flags->space_sign) {
        prefix[prefix_len++] = ' ';
    }
    
    if (flags->alt_form && abs_num != 0) {
        if (base == 8) {
            prefix[prefix_len++] = '0';
        } else if (base == 16) {
            prefix[prefix_len++] = '0';
            prefix[prefix_len++] = uppercase ? 'X' : 'x';
        }
    }
    
    int total_len = prefix_len + zeros_needed + num_len;
    int padding = flags->width > total_len ? flags->width - total_len : 0;
    
    if (!flags->left_align && !flags->zero_pad && padding > 0) {
        print_repeat(' ', padding);
    }
    
    for (int i = 0; i < prefix_len; i++) {
        print_char(prefix[i]);
    }
    
    if (!flags->left_align && flags->zero_pad && padding > 0) {
        print_repeat('0', padding);
    }

    print_repeat('0', zeros_needed);
    
    for (int i = 0; i < num_len; i++) {
        print_char(buf[i]);
    }

    if (flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
}

static void format_pointer(void *ptr, format_flags_t *flags) {
    if (!ptr) {
        format_string("(nil)", flags);
        return;
    }
    
    unsigned long long addr = (unsigned long long)(uintptr_t)ptr;
    char buf[32];
    int len = num_to_str(addr, buf, 16, false);
    
    int total_len = len + 2;
    int padding = flags->width > total_len ? flags->width - total_len : 0;
    
    if (!flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
    
    print_string("0x");
    print_string(buf);
    
    if (flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
}

void format_float(double num, format_flags_t *flags) {
    char buffer[64];
    int pos = 0;

    if (num < 0) {
        buffer[pos++] = '-';
        num = -num;
    } else if (flags->show_sign) {
        buffer[pos++] = '+';
    } else if (flags->space_sign) {
        buffer[pos++] = ' ';
    }

    int int_part = (int)num;
    char num_buf[32];
    int num_len = num_to_str(int_part < 0 ? -int_part : int_part, num_buf, 10, false);
    memcpy(&buffer[pos], num_buf, num_len);
    pos += num_len;

    buffer[pos++] = '.';
    
    double fractional_part = num - int_part;
    int precision = flags->has_precision ? flags->precision : 6;
    
    for (int i = 0; i < precision; i++) {
        fractional_part *= 10;
        int digit = (int)fractional_part;
        buffer[pos++] = '0' + digit;
        fractional_part -= digit;
    }

    buffer[pos] = '\0';
    
    int total_len = pos;
    int padding = flags->width > total_len ? flags->width - total_len : 0;
    
    if (!flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
    
    print_string(buffer);
    
    if (flags->left_align && padding > 0) {
        print_repeat(' ', padding);
    }
}

typedef struct {
    char* buffer;
    size_t size;
    size_t position;
} buffer_state_t;

static void buffer_write_char(char c, buffer_state_t* state) {
    if (state->position < state->size - 1) {
        state->buffer[state->position++] = c;
    } else if (state->position < state->size) {
        state->buffer[state->position++] = '\0';
    }
}

static void buffer_write_string(const char* str, buffer_state_t* state) {
    if (!str) str = "(null)";
    
    while (*str && state->position < state->size - 1) {
        state->buffer[state->position++] = *str++;
    }
    
    if (state->position < state->size) {
        state->buffer[state->position] = '\0';
    }
}

static void buffer_write_number(long long num, int base, bool uppercase, buffer_state_t* state) {
    char buffer[65];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';
    
    bool negative = false;
    unsigned long long unum;
    
    if (num < 0 && base == 10) {
        negative = true;
        unum = -num;
    } else {
        unum = num;
    }
    
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    
    do {
        *--ptr = digits[unum % base];
        unum /= base;
    } while (unum > 0);
    
    if (negative) {
        *--ptr = '-';
    }
    
    buffer_write_string(ptr, state);
}

int vsprintf(char* buffer, const char* format, va_list args) {
    buffer_state_t state = {buffer, SIZE_MAX, 0};
    
    const char *fmt = format;
    
    while (*fmt) {
        if (*fmt != '%') {
            buffer_write_char(*fmt, &state);
            fmt++;
            continue;
        }
        
        fmt++;
        
        if (*fmt == '%') {
            buffer_write_char('%', &state);
            fmt++;
            continue;
        }
        
        format_flags_t flags;
        fmt = parse_flags(fmt, &flags);
        
        switch (*fmt) {
            case 'c': {
                int c = va_arg(args, int);
                buffer_write_char((char)c, &state);
                break;
            }
            
            case 's': {
                char *str = va_arg(args, char*);
                buffer_write_string(str, &state);
                break;
            }
            
            case 'd':
            case 'i': {
                int num = va_arg(args, int);
                buffer_write_number(num, 10, false, &state);
                break;
            }
            
            case 'u': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 10, false, &state);
                break;
            }
            
            case 'o': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 8, false, &state);
                break;
            }
            
            case 'x': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 16, false, &state);
                break;
            }
            
            case 'X': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 16, true, &state);
                break;
            }
            
            case 'p': {
                void *ptr = va_arg(args, void*);
                buffer_write_string("0x", &state);
                buffer_write_number((unsigned long)ptr, 16, false, &state);
                break;
            }
            
            case 'n': {
                int *ptr = va_arg(args, int*);
                if (ptr) *ptr = state.position;
                break;
            }
            
            default:
                buffer_write_char('%', &state);
                buffer_write_char(*fmt, &state);
                break;
        }
        
        fmt++;
    }
    
    if (state.position < state.size) {
        state.buffer[state.position] = '\0';
    }
    
    return state.position;
}

int vsnprintf(char* buffer, size_t size, const char* format, va_list args) {
    buffer_state_t state = {buffer, size, 0};
    
    const char *fmt = format;
    
    while (*fmt) {
        if (*fmt != '%') {
            buffer_write_char(*fmt, &state);
            fmt++;
            continue;
        }
        
        fmt++;
        
        if (*fmt == '%') {
            buffer_write_char('%', &state);
            fmt++;
            continue;
        }
        
        format_flags_t flags;
        fmt = parse_flags(fmt, &flags);
        
        switch (*fmt) {
            case 'c': {
                int c = va_arg(args, int);
                buffer_write_char((char)c, &state);
                break;
            }
            
            case 's': {
                char *str = va_arg(args, char*);
                buffer_write_string(str, &state);
                break;
            }
            
            case 'd':
            case 'i': {
                int num = va_arg(args, int);
                buffer_write_number(num, 10, false, &state);
                break;
            }
            
            case 'u': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 10, false, &state);
                break;
            }
            
            case 'o': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 8, false, &state);
                break;
            }
            
            case 'x': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 16, false, &state);
                break;
            }
            
            case 'X': {
                unsigned int num = va_arg(args, unsigned int);
                buffer_write_number(num, 16, true, &state);
                break;
            }
            
            case 'p': {
                void *ptr = va_arg(args, void*);
                buffer_write_string("0x", &state);
                buffer_write_number((unsigned long)ptr, 16, false, &state);
                break;
            }
            
            case 'n': {
                int *ptr = va_arg(args, int*);
                if (ptr) *ptr = state.position;
                break;
            }
            
            default:
                buffer_write_char('%', &state);
                buffer_write_char(*fmt, &state);
                break;
        }
        
        fmt++;
    }
    
    if (state.position < state.size) {
        state.buffer[state.position] = '\0';
    } else if (size > 0) {
        state.buffer[size - 1] = '\0';
    }
    
    return state.position;
}

int sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsprintf(buffer, format, args);
    va_end(args);
    return result;
}

int snprintf(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    const char *fmt = format;
    
    while (*fmt) {
        if (*fmt != '%') {
            print_char(*fmt);
            count++;
            fmt++;
            continue;
        }
        
        fmt++;
        
        if (*fmt == '%') {
            print_char('%');
            count++;
            fmt++;
            continue;
        }
        
        format_flags_t flags;
        fmt = parse_flags(fmt, &flags);
        
        switch (*fmt) {
            case 'c': {
                int c = va_arg(args, int);
                format_char((char)c, &flags);
                count++;
                break;
            }
            
            case 's': {
                char *str = va_arg(args, char*);
                format_string(str, &flags);
                count += str ? strlen(str) : 6; // "(null)"
                break;
            }
            
            case 'd':
            case 'i': {
                int num = va_arg(args, int);
                format_number(num, &flags, 10, false, true);
                count++;
                break;
            }
            
            case 'l': {
                if (*(fmt + 1) == 'l') {
                    fmt += 2;
                    if (*fmt == 'x') {
                        unsigned long long num = va_arg(args, unsigned long long);
                        format_number(num, &flags, 16, true, false);
                        count++;
                    }
                    else if (*fmt == 'X') {
                        unsigned int num = va_arg(args, unsigned int);
                        format_number(num, &flags, 16, false, false);
                        count++;
                    }
                    else if (*fmt == 'd') {
                        long long num = va_arg(args, long long);
                        format_number(num, &flags, 10, false, true);
                        count++;
                    }
                }
                break;
            }

            case 'f': {
                double num = va_arg(args, double);
                format_float(num, &flags);
                count++;
                break;
            }

            case 'u': {
                unsigned int num = va_arg(args, unsigned int);
                format_number(num, &flags, 10, false, false);
                count++;
                break;
            }
            
            case 'o': {
                unsigned int num = va_arg(args, unsigned int);
                format_number(num, &flags, 8, false, false);
                count++;
                break;
            }
            
            case 'x': {
                unsigned int num = va_arg(args, unsigned int);
                format_number(num, &flags, 16, false, false);
                count++;
                break;
            }
            
            case 'X': {
                unsigned int num = va_arg(args, unsigned int);
                format_number(num, &flags, 16, true, false);
                count++;
                break;
            }
            
            case 'p': {
                void *ptr = va_arg(args, void*);
                format_pointer(ptr, &flags);
                count++;
                break;
            }
            
            case 'n': {
                int *ptr = va_arg(args, int*);
                if (ptr) *ptr = count;
                break;
            }
            
            default:
                print_char('%');
                print_char(*fmt);
                count += 2;
                break;
        }
        
        fmt++;
    }
    
    va_end(args);
    return count;
}
