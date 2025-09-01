#include "mouse.h"
#include <asm/io.h>
#include "../../kernel/idt/idt.h"

mouse_state_t mouse_state = {0};
uint32_t mouse_screen_width = SCREEN_WIDTH;
uint32_t mouse_screen_height = SCREEN_HEIGHT;

static void mouse_wait(bool type) {
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(MOUSE_STATUS_PORT);
        if (type) {
            if (!(status & 0x02)) return;
        } else {
            if (status & 0x01) return;
        }
    }
}

void mouse_write(uint8_t command) {
    mouse_wait(true);
    outb(MOUSE_COMMAND_PORT, 0xD4);
    mouse_wait(true);
    outb(MOUSE_DATA_PORT, command);
}

uint8_t mouse_read() {
    mouse_wait(false);
    return inb(MOUSE_DATA_PORT);
}

void mouse_handler() {
    static uint8_t cycle = 0;
    static uint8_t packet[3];
    
    uint8_t data = mouse_read();
    
    if (cycle == 0 && (data & 0x08)) {
        packet[0] = data;
        cycle++;
    } else if (cycle == 1) {
        packet[1] = data;
        cycle++;
    } else if (cycle == 2) {
        packet[2] = data;
        
        int8_t x_movement = packet[1];
        int8_t y_movement = packet[2];
        
        mouse_state.x += x_movement;
        mouse_state.y -= y_movement;
        
        if (mouse_state.bounded) {
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.x >= (int32_t)mouse_screen_width) 
                mouse_state.x = mouse_screen_width - 1;
            if (mouse_state.y >= (int32_t)mouse_screen_height) 
                mouse_state.y = mouse_screen_height - 1;
        }
        
        mouse_state.state = MOUSE_NONE;
        if (packet[0] & 0x01) mouse_state.state |= MOUSE_LEFT;
        if (packet[0] & 0x02) mouse_state.state |= MOUSE_RIGHT;
        if (packet[0] & 0x04) mouse_state.state |= MOUSE_MIDDLE;
        
        mouse_state.updated = true;
        cycle = 0;
    }
}

void mouse_init() {
    mouse_state.x = mouse_screen_width / 2;
    mouse_state.y = mouse_screen_height / 2;
    mouse_state.state = MOUSE_NONE;
    mouse_state.updated = false;
    mouse_state.bounded = true;
    
    mouse_wait(true);
    outb(MOUSE_COMMAND_PORT, 0xA8);
    
    mouse_wait(true);
    outb(MOUSE_COMMAND_PORT, 0x20);
    mouse_wait(false);
    uint8_t status = inb(MOUSE_DATA_PORT) | 0x02;
    mouse_wait(true);
    outb(MOUSE_COMMAND_PORT, 0x60);
    mouse_wait(true);
    outb(MOUSE_DATA_PORT, status);
    
    mouse_write(0xF6);
    mouse_read();
    mouse_write(0xF4);
    mouse_read();
    
    register_irq_handler(12, &mouse_handler);
    
    printf("Mouse initialized for %dx%d resolution\n", 
           mouse_screen_width, mouse_screen_height);
}

void mouse_set_bounds(uint32_t width, uint32_t height) {
    mouse_screen_width = width;
    mouse_screen_height = height;
    
    if (mouse_state.bounded) {
        if (mouse_state.x >= (int32_t)width) mouse_state.x = width - 1;
        if (mouse_state.y >= (int32_t)height) mouse_state.y = height - 1;
    }
}

void mouse_set_position(int32_t x, int32_t y) {
    mouse_state.x = x;
    mouse_state.y = y;
    
    if (mouse_state.bounded) {
        if (mouse_state.x < 0) mouse_state.x = 0;
        if (mouse_state.y < 0) mouse_state.y = 0;
        if (mouse_state.x >= (int32_t)mouse_screen_width) 
            mouse_state.x = mouse_screen_width - 1;
        if (mouse_state.y >= (int32_t)mouse_screen_height) 
            mouse_state.y = mouse_screen_height - 1;
    }
    
    mouse_state.updated = true;
}

void mouse_set_bounding(bool bounded) {
    mouse_state.bounded = bounded;
}

float mouse_get_normalized_x(void) {
    return (float)mouse_state.x / (float)(mouse_screen_width - 1);
}

float mouse_get_normalized_y(void) {
    return (float)mouse_state.y / (float)(mouse_screen_height - 1);
}

mouse_state_t mouse_get_state() {
    mouse_state_t state = mouse_state;
    mouse_state.updated = false;
    return state;
}

bool mouse_is_button_pressed(mouse_button_state_t button) {
    return (mouse_state.state & button) != 0;
}