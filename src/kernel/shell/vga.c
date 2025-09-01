#include "vga.h"
#include <stdint.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define PLANE_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 8)

static uint8_t plane0[PLANE_SIZE];
static uint8_t plane1[PLANE_SIZE];
static uint8_t plane2[PLANE_SIZE];
static uint8_t plane3[PLANE_SIZE];
static int dirty_x1 = SCREEN_WIDTH, dirty_y1 = SCREEN_HEIGHT;
static int dirty_x2 = 0, dirty_y2 = 0;
static int dirty = 0;

static void add_dirty_region(int x, int y, int width, int height) {
    if (!dirty) {
        dirty_x1 = x;
        dirty_y1 = y;
        dirty_x2 = x + width;
        dirty_y2 = y + height;
        dirty = 1;
        return;
    }
    
    if (x < dirty_x1) dirty_x1 = x;
    if (y < dirty_y1) dirty_y1 = y;
    if (x + width > dirty_x2) dirty_x2 = x + width;
    if (y + height > dirty_y2) dirty_y2 = y + height;
}

void vga_init() {
    outb(0x3C2, 0xE3);
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x06);

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
    
    for (int i = 0; i < PLANE_SIZE; i++) {
        plane0[i] = 0;
        plane1[i] = 0;
        plane2[i] = 0;
        plane3[i] = 0;
    }
    dirty = 0;

    uint8_t palette[16][3] = {
        {0x00, 0x00, 0x00}, {0x00, 0x00, 0xAA}, {0x00, 0xAA, 0x00}, {0x00, 0xAA, 0xAA},
        {0xAA, 0x00, 0x00}, {0xAA, 0x00, 0xAA}, {0xAA, 0x55, 0x00}, {0xAA, 0xAA, 0xAA},
        {0x55, 0x55, 0x55}, {0x55, 0x55, 0xFF}, {0x55, 0xFF, 0x55}, {0x55, 0xFF, 0xFF},
        {0xFF, 0x55, 0x55}, {0xFF, 0x55, 0xFF}, {0xFF, 0xFF, 0x55}, {0xFF, 0xFF, 0xFF}
    };
    for (int i = 0; i < 16; i++) {
        vga_set_palette_color(i, palette[i][0], palette[i][1], palette[i][2]);
    }
}

void vga_clear_buffer(uint8_t color) {
    uint8_t pattern0 = (color & 1) ? 0xFF : 0x00;
    uint8_t pattern1 = (color & 2) ? 0xFF : 0x00;
    uint8_t pattern2 = (color & 4) ? 0xFF : 0x00;
    uint8_t pattern3 = (color & 8) ? 0xFF : 0x00;

    for (int i = 0; i < PLANE_SIZE; i++) {
        plane0[i] = pattern0;
        plane1[i] = pattern1;
        plane2[i] = pattern2;
        plane3[i] = pattern3;
    }
    
    dirty_x1 = 0;
    dirty_y1 = 0;
    dirty_x2 = SCREEN_WIDTH;
    dirty_y2 = SCREEN_HEIGHT;
    dirty = 1;
}

void vga_draw_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    int byte_offset = y * (SCREEN_WIDTH / 8) + (x / 8);
    int bit_mask = 0x80 >> (x % 8);

    if (color & 1) plane0[byte_offset] |= bit_mask;
    else plane0[byte_offset] &= ~bit_mask;

    if (color & 2) plane1[byte_offset] |= bit_mask;
    else plane1[byte_offset] &= ~bit_mask;

    if (color & 4) plane2[byte_offset] |= bit_mask;
    else plane2[byte_offset] &= ~bit_mask;

    if (color & 8) plane3[byte_offset] |= bit_mask;
    else plane3[byte_offset] &= ~bit_mask;

    add_dirty_region(x, y, 1, 1);
}

void vga_swap_buffers() {
    if (!dirty) return;
    
    dirty_x1 = dirty_x1 < 0 ? 0 : dirty_x1;
    dirty_y1 = dirty_y1 < 0 ? 0 : dirty_y1;
    dirty_x2 = dirty_x2 > SCREEN_WIDTH ? SCREEN_WIDTH : dirty_x2;
    dirty_y2 = dirty_y2 > SCREEN_HEIGHT ? SCREEN_HEIGHT : dirty_y2;
    
    if (dirty_x1 >= dirty_x2 || dirty_y1 >= dirty_y2) {
        dirty = 0;
        return;
    }
    
    uint8_t* vga_mem = (uint8_t*)0xA0000;
    int start_byte_x = dirty_x1 / 8;
    int end_byte_x = (dirty_x2 + 7) / 8;
    int bytes_per_line = end_byte_x - start_byte_x;
    
    for (int plane = 0; plane < 4; plane++) {
        outb(0x3C4, 0x02);
        outb(0x3C5, 1 << plane);
        
        uint8_t* plane_data;
        switch (plane) {
            case 0: plane_data = plane0; break;
            case 1: plane_data = plane1; break;
            case 2: plane_data = plane2; break;
            case 3: plane_data = plane3; break;
        }
        
        for (int y = dirty_y1; y < dirty_y2; y++) {
            int line_offset = y * (SCREEN_WIDTH / 8) + start_byte_x;
            for (int x = 0; x < bytes_per_line; x++) {
                vga_mem[line_offset + x] = plane_data[line_offset + x];
            }
        }
    }
    
    dirty = 0;
}

void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        vga_draw_pixel(x1, y1, color);
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
    
    for (int y = y1; y < y2; y++) {
        vga_draw_line((int)xa, y, (int)xb, y, color);
        xa += dx13;
        xb += dx12;
    }

    xb = x2;
    for (int y = y2; y <= y3; y++) {
        vga_draw_line((int)xa, y, (int)xb, y, color);
        xa += dx13;
        xb += dx23;
    }
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
}

void vga_fill_circle(int center_x, int center_y, int radius, uint8_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    
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

/**
 * Рисует изображение из массива данных на экране
 * @param data - массив с данными пикселей (формат: width * height байт)
 * @param x - координата X левого верхнего угла
 * @param y - координата Y левого верхнего угла
 * @param width - ширина изображения
 * @param height - высота изображения
 * @param transparent_color - цвет, который считается прозрачным (не рисуется)
 */
void vga_draw_from_array(const uint8_t* data, int x, int y, int width, int height, uint8_t transparent_color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            uint8_t color = data[j * width + i];
            if (color != transparent_color) {
                vga_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

/**
 * Рисует изображение из массива данных с возможностью отражения
 * @param data - массив с данными пикселей
 * @param x - координата X левого верхнего угла
 * @param y - координата Y левого верхнего угла
 * @param width - ширина изображения
 * @param height - высота изображения
 * @param transparent_color - прозрачный цвет
 * @param flip_horizontal - отразить по горизонтали (1 - да, 0 - нет)
 * @param flip_vertical - отразить по вертикали (1 - да, 0 - нет)
 */
void vga_draw_from_array_flipped(const uint8_t* data, int x, int y, int width, int height, 
                                uint8_t transparent_color, int flip_horizontal, int flip_vertical) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int src_x = flip_horizontal ? (width - 1 - i) : i;
            int src_y = flip_vertical ? (height - 1 - j) : j;
            
            uint8_t color = data[src_y * width + src_x];
            if (color != transparent_color) {
                vga_draw_pixel(x + i, y + j, color);
            }
        }
    }
}