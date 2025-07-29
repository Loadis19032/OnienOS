#pragma once

#include <stdint.h>
#include <stdbool.h>

struct registers;

void init_keyboard();
void keyboardHandler(struct InterruptRegisters *regs);
char* fgets(char* buf, uint32_t size);
char getch();
bool kbhit();