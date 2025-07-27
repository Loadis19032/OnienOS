#include "stdint.h"
#include "stddef.h"
#include <stdio.h>
#include "idt/idt.h"
#include <asm/io.h>

void handler(struct registers* regs) {
    uint8_t scancode = inb(0x60);

    printf("Scancode: 0x%x\n", scancode);

    outb(0x20, 0x20);
}

void kmain() {
    idt_init();
    register_irq_handler(1, handler);
    outb(0x21, inb(0x21) & ~0x02);

    printf("Hello 64-bit World!\n\n");

    for (;;);
}