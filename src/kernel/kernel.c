#include "stdint.h"
#include "stddef.h"
#include <stdio.h>
#include "idt/idt.h"
#include <asm/io.h>
#include <string.h>
#include "mm/mem.h"
#include "../drivers/keyboard/keyboard.h"
#include <math.h>
#include <stdlib.h>
 
bool check_win();

#define WIDTH 10
#define HEIGHT 10
#define MINES 15

#define COVERED 0x10
#define FLAGGED 0x20
#define MINE    0x40

uint8_t board[HEIGHT][WIDTH];
bool game_over = false;
bool first_move = true;
int cursor_x = 0, cursor_y = 0;

void init_board() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            board[y][x] = COVERED;
        }
    }
}

void place_mines(int avoid_x, int avoid_y) {
    int mines_placed = 0;
    while (mines_placed < MINES) {
        int x = rand() % WIDTH;
        int y = rand() % HEIGHT;
        
        if ((abs(x - avoid_x) <= 1 && abs(y - avoid_y) <= 1) || 
             (board[y][x] & MINE)) {
            continue;
        }
        
        board[y][x] |= MINE;
        mines_placed++;
    }
}

int count_adjacent_mines(int x, int y) {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                if (board[ny][nx] & MINE) count++;
            }
        }
    }
    return count;
}

void reveal(int x, int y) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    if (!(board[y][x] & COVERED)) return;
    if (board[y][x] & FLAGGED) return;
    
    board[y][x] &= ~COVERED;
    
    if (board[y][x] & MINE) {
        game_over = true;
        return;
    }
    
    if (count_adjacent_mines(x, y) == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx != 0 || dy != 0) {
                    reveal(x + dx, y + dy);
                }
            }
        }
    }
}

void draw_cell(int x, int y, bool is_cursor) {
    if (is_cursor) {
        setcolor(VGA_COLOR(VGA_WHITE, VGA_BLACK));
    } else {
        setcolor(VGA_COLOR(VGA_BLACK, VGA_LGRAY));
    }

    if (game_over && (board[y][x] & MINE)) {
        setcolor(VGA_COLOR(VGA_RED, VGA_BLACK));
        putchar('*');
        return;
    }
    
    if (board[y][x] & COVERED) {
        if (board[y][x] & FLAGGED) {
            if (is_cursor) {
                setcolor(VGA_COLOR(VGA_YELLOW, VGA_WHITE));
            } else {
                setcolor(VGA_COLOR(VGA_YELLOW, VGA_BLACK));
            }
            putchar('F');
        } else {
            if (is_cursor) {
                setcolor(VGA_COLOR(VGA_BLACK, VGA_WHITE));
            }
            putchar('#');
        }
        return;
    }
    
    int mines = count_adjacent_mines(x, y);
    if (mines > 0) {
        uint8_t color;
        switch (mines) {
            case 1: color = VGA_BLUE; break;
            case 2: color = VGA_GREEN; break;
            case 3: color = VGA_RED; break;
            case 4: color = VGA_MAGENTA; break;
            default: color = VGA_YELLOW; break;
        }
        if (is_cursor) {
            setcolor(VGA_COLOR(color, VGA_WHITE));
        } else {
            setcolor(VGA_COLOR(color, VGA_BLACK));
        }
        printf("%d", mines);
    } else {
        if (is_cursor) {
            setcolor(VGA_COLOR(VGA_BLACK, VGA_WHITE));
        } else {
            setcolor(VGA_COLOR(VGA_BLACK, VGA_LGRAY));
        }
        putchar('.');
    }
}

void draw_board() {
    setcolor(VGA_COLOR(VGA_BLACK, VGA_WHITE));
    clear();
    
    printf("  Minesweeper (Mines: %d)\n\n  ", MINES);
    for (int x = 0; x < WIDTH; x++) {
        printf(" %c", 'A' + x);
    }
    printf("\n");
    
    for (int y = 0; y < HEIGHT; y++) {
        printf("%2d ", y + 1);
        for (int x = 0; x < WIDTH; x++) {
            draw_cell(x, y, (x == cursor_x && y == cursor_y));
            putchar(' ');
        }
        printf("\n");
    }
    
    if (game_over) {
        printf("\n Game Over! Press R to restart.\n");
    } else if (check_win()) {
        printf("\n You Win! Press R to restart.\n");
    } else {
        printf("\n WASD:Move  F:Flag  Space:Reveal  Q:Quit\n");
    }
}

bool check_win() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (!(board[y][x] & MINE) && (board[y][x] & COVERED)) {
                return false;
            }
        }
    }
    return true;
}

void game_loop() {
    srand(434);
    init_board();
    
    while (1) {
        draw_board();
        char key = getch();
        
        if (game_over || check_win()) {
            if (key == 'r' || key == 'R') {
                game_over = false;
                first_move = true;
                init_board();
            } else if (key == 'q' || key == 'Q') {
                return;
            }
            continue;
        }
        
        switch (key) {
            case 'w': case 'W': if (cursor_y > 0) cursor_y--; break;
            case 's': case 'S': if (cursor_y < HEIGHT-1) cursor_y++; break;
            case 'a': case 'A': if (cursor_x > 0) cursor_x--; break;
            case 'd': case 'D': if (cursor_x < WIDTH-1) cursor_x++; break;
            case 'f': case 'F':
                if (board[cursor_y][cursor_x] & COVERED) {
                    board[cursor_y][cursor_x] ^= FLAGGED;
                }
                break;
            case ' ':
                if (first_move) {
                    place_mines(cursor_x, cursor_y);
                    first_move = false;
                }
                if (!(board[cursor_y][cursor_x] & FLAGGED)) {
                    reveal(cursor_x, cursor_y);
                }
                break;
            case 'q': case 'Q':
                return;
        }
    }
}

void kmain(uint32_t magic, uint32_t info) {
    idt_init();
    init_keyboard();
    
    uint64_t total_ram = 0;
    uint64_t heap_start = 0x400000;
    
    mem_init(32*1024*1024, (void*)heap_start);

    game_loop();

    for (;;);
}