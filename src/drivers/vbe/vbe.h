#ifndef VBE_H
#define VBE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    int x;
    int y;
    int width;
    int height;
} dirty_region_t;

int init_lfb();
void put_pixel(int x, int y, uint32_t color);
void draw_char(int x, int y, char c, uint32_t color);
void draw_string(int x, int y, const char* str, uint32_t color);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void fill_rect(int x, int y, int width, int height, uint32_t color);
void draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
void draw_circle(int center_x, int center_y, int radius, uint32_t color);
void fill_circle(int center_x, int center_y, int radius, uint32_t color);
void draw_ellipse(int center_x, int center_y, int a, int b, uint32_t color);
void fill_ellipse(int center_x, int center_y, int a, int b, uint32_t color);
void draw_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color);
void fill_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color);
void draw_polygon(const int* points, int num_points, uint32_t color);
void fill_polygon(const int* points, int num_points, uint32_t color);
void swap_buffers();
void clear_screen(uint32_t color);
uint32_t get_screen_width();
uint32_t get_screen_height();
uint32_t* get_framebuffer();
void enable_vsync();
void disable_vsync();
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void add_dirty_region(int x, int y, int width, int height);
void reset_dirty_regions();

#endif