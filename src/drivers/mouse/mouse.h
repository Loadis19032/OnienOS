#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stdbool.h>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1024
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 768
#endif

typedef enum {
    MOUSE_NONE = 0,
    MOUSE_LEFT = (1 << 0),
    MOUSE_RIGHT = (1 << 1),
    MOUSE_MIDDLE = (1 << 2)
} mouse_button_state_t;

typedef struct {
    int32_t x;                 
    int32_t y;                 
    mouse_button_state_t state;
    bool updated;              
    bool bounded;              
} mouse_state_t;

void mouse_init(void);
void mouse_handler(void);

void mouse_set_bounds(uint32_t width, uint32_t height);
void mouse_set_position(int32_t x, int32_t y);
void mouse_set_bounding(bool bounded);

float mouse_get_normalized_x(void);
float mouse_get_normalized_y(void);

mouse_state_t mouse_get_state(void);
bool mouse_is_button_pressed(mouse_button_state_t button);

extern mouse_state_t mouse_state;
extern uint32_t mouse_screen_width;
extern uint32_t mouse_screen_height;

#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64
#define MOUSE_COMMAND_PORT 0x64

#endif