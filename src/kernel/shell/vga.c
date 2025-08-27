#include "vga.h"
#include <stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BUFFER_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 8)

static uint8_t back_buffer[BUFFER_SIZE];
static uint8_t front_buffer[BUFFER_SIZE];
static uint8_t pixel_colors[SCREEN_WIDTH * SCREEN_HEIGHT];
static int double_buffering = 1;

static int dirty_regions[4][4];
static int dirty_count = 0;

static void fast_memcpy(uint8_t* dest, const uint8_t* src, int size) {
    int i = 0;
    for (; i + 7 < size; i += 8) {
        asm volatile (
            "movq (%1), %%mm0\n"
            "movq %%mm0, (%0)\n"
            : : "r"(dest + i), "r"(src + i) : "memory"
        );
    }
    asm volatile ("emms" : : : "memory");
    for (; i < size; i++) {
        dest[i] = src[i];
    }
}

static void add_dirty_region(int x, int y, int width, int height) {
    if (dirty_count >= 4) {
        dirty_count = 1;
        dirty_regions[0][0] = 0;
        dirty_regions[0][1] = 0;
        dirty_regions[0][2] = SCREEN_WIDTH;
        dirty_regions[0][3] = SCREEN_HEIGHT;
        return;
    }
    dirty_regions[dirty_count][0] = x;
    dirty_regions[dirty_count][1] = y;
    dirty_regions[dirty_count][2] = x + width;
    dirty_regions[dirty_count][3] = y + height;
    dirty_count++;
}

void vga_init() {
    outb(0x3C2, 0xE3);
    outb(0x3C4, 0x00);
    outb(0x3C5, 0x03);
    outb(0x3C4, 0x01);
    outb(0x3C5, 0x01);
    outb(0x3C4, 0x02);
    outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03);
    outb(0x3C5, 0x00);
    outb(0x3C4, 0x04);
    outb(0x3C5, 0x06);

    static const uint8_t crtc[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
        0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
        0xFF
    };

    outb(0x3D4, 0x11);
    outb(0x3D5, inb(0x3D5) & ~0x80);

    for (uint8_t i = 0; i < sizeof(crtc); i++) {
        outb(0x3D4, i);
        outb(0x3D5, crtc[i]);
    }

    outb(0x3D4, 0x0C); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0D); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0C); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0D); outb(0x3D5, 0x00);
    outb(0x3CE, 0x00); outb(0x3CF, 0x00);
    outb(0x3CE, 0x01); outb(0x3CF, 0x00);
    outb(0x3CE, 0x02); outb(0x3CF, 0x00);
    outb(0x3CE, 0x03); outb(0x3CF, 0x00);
    outb(0x3CE, 0x04); outb(0x3CF, 0x00);
    outb(0x3CE, 0x05); outb(0x3CF, 0x00);
    outb(0x3CE, 0x06); outb(0x3CF, 0x05);
    outb(0x3CE, 0x07); outb(0x3CF, 0x0F);
    outb(0x3CE, 0x08); outb(0x3CF, 0xFF);

    for (uint8_t i = 0; i < 16; i++) {
        (void)inb(0x3DA);
        outb(0x3C0, i);
        outb(0x3C0, i);
    }

    (void)inb(0x3DA); outb(0x3C0, 0x10); outb(0x3C0, 0x01);
    (void)inb(0x3DA); outb(0x3C0, 0x11); outb(0x3C0, 0x00);
    (void)inb(0x3DA); outb(0x3C0, 0x12); outb(0x3C0, 0x0F);
    (void)inb(0x3DA); outb(0x3C0, 0x13); outb(0x3C0, 0x00);
    (void)inb(0x3DA); outb(0x3C0, 0x14); outb(0x3C0, 0x00);
    (void)inb(0x3DA);
    outb(0x3C0, 0x20);
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        back_buffer[i] = 0;
        front_buffer[i] = 0;
    }
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixel_colors[i] = 0;
    }
    dirty_count = 0;
}

void vga_clear_buffer(uint8_t color) {
    uint8_t pattern = 0x00;
    if (color & 0x01) pattern |= 0x55;
    if (color & 0x02) pattern |= 0xAA;
    if (color & 0x04) pattern |= 0x55;
    if (color & 0x08) pattern |= 0xAA;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        back_buffer[i] = pattern;
    }
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixel_colors[i] = color;
    }
    dirty_count = 1;
    dirty_regions[0][0] = 0;
    dirty_regions[0][1] = 0;
    dirty_regions[0][2] = SCREEN_WIDTH;
    dirty_regions[0][3] = SCREEN_HEIGHT;
}

void vga_draw_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    pixel_colors[y * SCREEN_WIDTH + x] = color;
    int byte_offset = y * 80 + (x / 8);
    int bit_mask = 0x80 >> (x % 8);
    back_buffer[byte_offset] |= bit_mask;
    add_dirty_region(x, y, 1, 1);
}

void vga_swap_buffers() {
    if (dirty_count == 0) return;
    
    if (dirty_count >= 3) {
        outb(0x3C4, 0x02);
        outb(0x3C5, 0x0F);
        uint8_t* vga = (uint8_t*)0xA0000;
        fast_memcpy(vga, back_buffer, BUFFER_SIZE);
    } else {
        for (int i = 0; i < dirty_count; i++) {
            int x1 = dirty_regions[i][0];
            int y1 = dirty_regions[i][1];
            int x2 = dirty_regions[i][2];
            int y2 = dirty_regions[i][3];
            
            for (int plane = 0; plane < 4; plane++) {
                outb(0x3C4, 0x02);
                outb(0x3C5, 1 << plane);
                uint8_t* vga = (uint8_t*)0xA0000;
                
                for (int y = y1; y < y2; y++) {
                    for (int x = x1; x < x2; x += 8) {
                        int byte_offset = y * 80 + (x / 8);
                        uint8_t byte_value = 0;
                        for (int i = 0; i < 8; i++) {
                            int px = x + i;
                            if (px < x2 && px < SCREEN_WIDTH) {
                                uint8_t color = pixel_colors[y * SCREEN_WIDTH + px];
                                if (color & (1 << plane)) {
                                    byte_value |= (0x80 >> i);
                                }
                            }
                        }
                        vga[byte_offset] = byte_value;
                    }
                }
            }
        }
    }
    dirty_count = 0;
}

uint8_t* vga_get_back_buffer() {
    return back_buffer;
}

void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int min_x = x1, max_x = x1, min_y = y1, max_y = y1;
    
    while (1) {
        vga_draw_pixel(x1, y1, color);
        if (x1 < min_x) min_x = x1;
        if (x1 > max_x) max_x = x1;
        if (y1 < min_y) min_y = y1;
        if (y1 > max_y) max_y = y1;
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    add_dirty_region(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
}

void vga_draw_rect(int x, int y, int width, int height, uint8_t color) {
    vga_draw_line(x, y, x + width, y, color);
    vga_draw_line(x + width, y, x + width, y + height, color);
    vga_draw_line(x + width, y + height, x, y + height, color);
    vga_draw_line(x, y + height, x, y, color);
}

void vga_fill_rect(int x, int y, int width, int height, uint8_t color) {
    for (int i = y; i < y + height; i++) {
        for (int j = x; j < x + width; j++) {
            vga_draw_pixel(j, i, color);
        }
    }
    add_dirty_region(x, y, width, height);
}

void vga_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint8_t color) {
    if (y1 > y2) { int tmp = x1; x1 = x2; x2 = tmp; tmp = y1; y1 = y2; y2 = tmp; }
    if (y1 > y3) { int tmp = x1; x1 = x3; x3 = tmp; tmp = y1; y1 = y3; y3 = tmp; }
    if (y2 > y3) { int tmp = x2; x2 = x3; x3 = tmp; tmp = y2; y2 = y3; y3 = tmp; }
    
    float dx13 = (y1 == y3) ? 0 : (float)(x3 - x1) / (y3 - y1);
    float dx12 = (y1 == y2) ? 0 : (float)(x2 - x1) / (y2 - y1);
    float dx23 = (y2 == y3) ? 0 : (float)(x3 - x2) / (y3 - y2);
    
    float xa = x1;
    float xb = x1;
    int min_x = x1, max_x = x1, min_y = y1, max_y = y3;
    
    for (int y = y1; y < y2; y++) {
        vga_draw_line((int)xa, y, (int)xb, y, color);
        if ((int)xa < min_x) min_x = (int)xa;
        if ((int)xb > max_x) max_x = (int)xb;
        xa += dx13;
        xb += dx12;
    }

    xb = x2;
    for (int y = y2; y <= y3; y++) {
        vga_draw_line((int)xa, y, (int)xb, y, color);
        if ((int)xa < min_x) min_x = (int)xa;
        if ((int)xb > max_x) max_x = (int)xb;
        xa += dx13;
        xb += dx23;
    }
    add_dirty_region(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
}

void vga_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint8_t color) {
    vga_draw_line(x1, y1, x2, y2, color);
    vga_draw_line(x2, y2, x3, y3, color);
    vga_draw_line(x3, y3, x1, y1, color);
}

void vga_draw_circle(int center_x, int center_y, int radius, uint8_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    int min_x = center_x - radius, max_x = center_x + radius;
    int min_y = center_y - radius, max_y = center_y + radius;
    
    while (y >= x) {
        vga_draw_pixel(center_x + x, center_y + y, color);
        vga_draw_pixel(center_x - x, center_y + y, color);
        vga_draw_pixel(center_x + x, center_y - y, color);
        vga_draw_pixel(center_x - x, center_y - y, color);
        vga_draw_pixel(center_x + y, center_y + x, color);
        vga_draw_pixel(center_x - y, center_y + x, color);
        vga_draw_pixel(center_x + y, center_y - x, color);
        vga_draw_pixel(center_x - y, center_y - x, color);
        
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
    add_dirty_region(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
}

void vga_fill_circle(int center_x, int center_y, int radius, uint8_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    int min_x = center_x - radius, max_x = center_x + radius;
    int min_y = center_y - radius, max_y = center_y + radius;
    
    while (y >= x) {
        vga_draw_line(center_x - x, center_y + y, center_x + x, center_y + y, color);
        vga_draw_line(center_x - x, center_y - y, center_x + x, center_y - y, color);
        vga_draw_line(center_x - y, center_y + x, center_x + y, center_y + x, color);
        vga_draw_line(center_x - y, center_y - x, center_x + y, center_y - x, color);
        
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
    add_dirty_region(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
}

void vga_draw_rounded_rect(int x, int y, int width, int height, int radius, uint8_t color) {
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;
    
    vga_draw_line(x + radius, y, x + width - radius, y, color);
    vga_draw_line(x + radius, y + height, x + width - radius, y + height, color);
    vga_draw_line(x, y + radius, x, y + height - radius, color);
    vga_draw_line(x + width, y + radius, x + width, y + height - radius, color);
    
    vga_draw_circle_arc(x + radius, y + radius, radius, 180, 270, color);
    vga_draw_circle_arc(x + width - radius, y + radius, radius, 270, 360, color);
    vga_draw_circle_arc(x + radius, y + height - radius, radius, 90, 180, color);
    vga_draw_circle_arc(x + width - radius, y + height - radius, radius, 0, 90, color);
}

void vga_fill_rounded_rect(int x, int y, int width, int height, int radius, uint8_t color) {
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;
    
    vga_fill_rect(x + radius, y, width - 2 * radius, height, color);
    vga_fill_rect(x, y + radius, radius, height - 2 * radius, color);
    vga_fill_rect(x + width - radius, y + radius, radius, height - 2 * radius, color);
    
    vga_fill_circle_quadrant(x + radius, y + radius, radius, 2, color);
    vga_fill_circle_quadrant(x + width - radius, y + radius, radius, 1, color);
    vga_fill_circle_quadrant(x + radius, y + height - radius, radius, 3, color);
    vga_fill_circle_quadrant(x + width - radius, y + height - radius, radius, 4, color);
}

void vga_draw_circle_arc(int center_x, int center_y, int radius, int start_angle, int end_angle, uint8_t color) {
    for (int angle = start_angle; angle <= end_angle; angle++) {
        float rad = angle * 3.14159 / 180.0;
        int px = center_x + radius * cos(rad);
        int py = center_y + radius * sin(rad);
        vga_draw_pixel(px, py, color);
    }
}

void vga_fill_circle_quadrant(int center_x, int center_y, int radius, int quadrant, uint8_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    
    while (y >= x) {
        switch (quadrant) {
            case 1:
                vga_draw_line(center_x, center_y - x, center_x + x, center_y - x, color);
                vga_draw_line(center_x, center_y - y, center_x + y, center_y - y, color);
                break;
            case 2:
                vga_draw_line(center_x - x, center_y - x, center_x, center_y - x, color);
                vga_draw_line(center_x - y, center_y - y, center_x, center_y - y, color);
                break;
            case 3:
                vga_draw_line(center_x - x, center_y + x, center_x, center_y + x, color);
                vga_draw_line(center_x - y, center_y + y, center_x, center_y + y, color);
                break;
            case 4:
                vga_draw_line(center_x, center_y + x, center_x + x, center_y + x, color);
                vga_draw_line(center_x, center_y + y, center_x + y, center_y + y, color);
                break;
        }
        
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r >> 2);
    outb(0x3C9, g >> 2);
    outb(0x3C9, b >> 2);
}