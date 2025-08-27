#pragma once

#include <stdint.h>

#define TASK_ERROR      0x1030
#define KERNEL_ERROR    0x1060
#define DRIVER_ERROR    0x1090
#define MM_ERROR        0x1220
#define IDT_ERROR       0x1250
#define IRQ_ERROR       0x1280
#define BOOT_ERROR      0x1310

void panic(const char* error_msg, uint32_t error_code);