#include "stdint.h"
#include <asm/io.h>
#include "vga.h"
#include <math.h>
#include <time.h>
#include <stdbool.h>

void _s() {
    vga_init();
    
    int x = 100, y = 100;
    int dx = 3, dy = 2;
    uint8_t color = VGA_RED;
    int radius = 20;
    
    while (!kbhit()) {
        vga_clear_buffer(VGA_BLACK);
        
        vga_fill_circle(x, y, radius, color);
        
        for (int i = 1; i <= 5; i++) {
            vga_draw_circle(x - dx*i/2, y - dy*i/2, radius - i*2, color - i);
        }
        
        vga_draw_line(x, y, x + dx*10, y + dy*10, VGA_LGRAY);

        x += dx;
        y += dy;
        
        if (x <= radius || x >= 640 - radius) {
            dx = -dx;
            color = (color + 1) % 16;
        }
        if (y <= radius || y >= 480 - radius) {
            dy = -dy;
            color = (color + 5) % 16;
        }
        
        vga_swap_buffers();
    }
}