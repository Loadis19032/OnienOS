#include "stdint.h"
#include <asm/io.h>
#include "vga.h"
#include <math.h>
#include <time.h>
#include <stdbool.h>

void _s() {
    vga_init();
    
    
    int x = 1;
    int y = 1;

    int countx = 1;
    int county = 1;

    while (1) {
        vga_clear_buffer(0);
        vga_fill_rect(x, y, 50, 50, 0x4);

        x += countx;
        y += county;

        if (x >= 640 || y >= 480) {
            countx -= 1;
            county += 1;
        }

        if (x == 0 || y == 0) {
            countx += 1;
            county -= 1;
        }

        vga_swap_buffers();
    }
}