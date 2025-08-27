#include "panic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void panic(const char* error_msg, uint32_t error_code) {
    setcolor(VGA_COLOR(VGA_BLUE, VGA_LGRAY));
    clear();

    char* code_msg = (char*)malloc(100);
    if (error_code == TASK_ERROR)
        code_msg = "TASK ERROR";
    if (error_code == KERNEL_ERROR)
        code_msg = "KERNEL ERROR";
    if (error_code == DRIVER_ERROR)
        code_msg = "DRIVER ERROR";

    printf("\n\n\n\n\n\t\t\t\t\t\t============== PANIC ==============\n");
    printf("\t\t\t\t\t\tError: %s\n\t\t\t\t\t\tCode: 0x%x (%s)", error_msg, error_code, code_msg);
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    setcolor(VGA_COLOR(VGA_LGRAY, VGA_BLACK));
    printf("    This operating system is in early development                              ");

    asm ("hlt");
}