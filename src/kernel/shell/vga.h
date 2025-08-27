#ifndef VGA12H_H
#define VGA12H_H

#include <stdint.h>
#include <stdio.h>

void vga_init();

void vga_swap_buffers();
void vga_clear_buffer(uint8_t color);

//width=640, height=480

void vga_draw_pixel(int x, int y, uint8_t color);
void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void vga_draw_rect(int x, int y, int width, int height, uint8_t color);
void vga_fill_rect(int x, int y, int width, int height, uint8_t color);
void vga_draw_rounded_rect(int x, int y, int width, int height, int radius, uint8_t color);
void vga_fill_rounded_rect(int x, int y, int width, int height, int radius, uint8_t color);
void vga_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint8_t color);
void vga_fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint8_t color);
void vga_draw_circle_arc(int center_x, int center_y, int radius, int start_angle, int end_angle, uint8_t color);
void vga_fill_circle_quadrant(int center_x, int center_y, int radius, int quadrant, uint8_t color);

uint8_t* vga_get_back_buffer();
void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

#endif