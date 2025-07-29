#include "keyboard.h"
#include <string.h>
#include <asm/io.h>
#include "../kernel/idt/idt.h"
#include <stdio.h>

#define KEYBOARD_BUFFER_SIZE 256
#define KEY_RELEASED_MASK 0x80

typedef struct {
    bool shift_left;
    bool shift_right;
    bool ctrl;
    bool alt;
    bool caps_lock;
} KeyboardModifiers;

static KeyboardModifiers keyboard_modifiers;
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t keyboard_head = 0;
static volatile uint32_t keyboard_tail = 0;

static const char scancode_lower[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+'
};

static const char scancode_upper[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+'
};

static bool is_uppercase() {
    return (keyboard_modifiers.shift_left || keyboard_modifiers.shift_right) ^ keyboard_modifiers.caps_lock;
}

static char scancode_to_ascii(uint8_t scancode) {
    if (scancode >= sizeof(scancode_lower)) return 0;
    return is_uppercase() ? scancode_upper[scancode] : scancode_lower[scancode];
}

void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    
    if (scancode & KEY_RELEASED_MASK) {
        uint8_t released_key = scancode & ~KEY_RELEASED_MASK;
        switch(released_key) {
            case 0x2A: keyboard_modifiers.shift_left = false; break;
            case 0x36: keyboard_modifiers.shift_right = false; break;
            case 0x1D: keyboard_modifiers.ctrl = false; break;
            case 0x38: keyboard_modifiers.alt = false; break;
        }
        return;
    }

    switch(scancode) {
        case 0x2A: keyboard_modifiers.shift_left = true; return;
        case 0x36: keyboard_modifiers.shift_right = true; return;
        case 0x1D: keyboard_modifiers.ctrl = true; return;
        case 0x38: keyboard_modifiers.alt = true; return;
        case 0x3A: keyboard_modifiers.caps_lock = !keyboard_modifiers.caps_lock; return;

        
    }

    char c = scancode_to_ascii(scancode);
    if (c == 0) return;

    //putchar(c);

    uint32_t next = (keyboard_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != keyboard_tail) {
        keyboard_buffer[keyboard_head] = c;
        keyboard_head = next;
    }

    outb(0x20, 0x20); // EOI
}

void init_keyboard() {
    keyboard_modifiers = (KeyboardModifiers){0};
    keyboard_head = keyboard_tail = 0;
    register_irq_handler(1, &keyboard_handler);
    outb(0x21, inb(0x21) & ~0x02);
    asm volatile ("sti");
}

bool kbhit() {
    return keyboard_head != keyboard_tail;
}

char getch() {
    while (!kbhit()) asm volatile("pause");
    char c = keyboard_buffer[keyboard_tail];
    keyboard_tail = (keyboard_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

char* fgets(char* buf, uint32_t size) {
    if (!buf || size == 0) return NULL;

    uint32_t i = 0;
    while (i < size - 1) {
        char c = getch();
        

        if (c == '\b') {
            if (i > 0) {
                i--;
                putchar('\b'); putchar(' '); putchar('\b');
            }
            continue;
        }
        
        buf[i++] = c;
        if (c == '\n') break;
    }
    
    buf[i] = '\0';
    return buf;
}