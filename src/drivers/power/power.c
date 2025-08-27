#include "power.h"
#include <stdint.h>
#include <asm/io.h>

#define ACPI_SHUTDOWN_PORT 0x604
#define ACPI_SHUTDOWN_VALUE 0x2000

#define KEYBOARD_CONTROLLER_PORT 0x64

static void io_wait(void) {
    outb(0x80, 0);
}

void shutdown(void) {
    outb(ACPI_SHUTDOWN_PORT, ACPI_SHUTDOWN_VALUE & 0xFF);
    outb(ACPI_SHUTDOWN_PORT + 1, ACPI_SHUTDOWN_VALUE >> 8);
    
    outb(0xF4, 0x00);
    
    while (1) {
        asm volatile ("hlt");
    }
}

void reboot(void) {
    uint8_t temp;
    
    do {
        temp = inb(KEYBOARD_CONTROLLER_PORT);
        if (temp & 0x1) {
            (void)inb(0x60);
        }
    } while (temp & 0x2);
    
    outb(KEYBOARD_CONTROLLER_PORT, 0xFE);
    io_wait();
    
    outb(0x64, 0xFE);
    
    asm volatile (
        "cli;"
        "lidt 0;"
        "int3;"
    );
    
    while (1) {
        asm volatile ("hlt");
    }
}
