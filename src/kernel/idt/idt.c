#include "idt.h"
#include <asm/io.h>
#include <stdio.h>
#include <string.h>

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;
static void (*irq_handlers[16])(struct registers*);

static const char* exception_messages[32] = {
    "Division By Zero", "Debug", "NMI", "Breakpoint", "Overflow",
    "Bound Range", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Overrun", "Invalid TSS",
    "Segment Not Present", "Stack Fault", "GPF", "Page Fault",
    "Reserved", "x87 Exception", "Alignment Check", "Machine Check",
    "SIMD Exception", "Virtualization", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Security Exception", "Reserved"
};

void idt_set_gate(uint8_t num, void* handler, uint8_t flags) {
    uint64_t addr = (uint64_t)handler;
    idt[num].base_low = addr & 0xFFFF;
    idt[num].base_mid = (addr >> 16) & 0xFFFF;
    idt[num].base_high = (addr >> 32) & 0xFFFFFFFF;
    idt[num].cs_selector = 0x08;
    idt[num].ist = 0;
    idt[num].attributes = flags;
    idt[num].reserved = 0;
}

void idt_init() {
    idtp.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idtp.base = (uint64_t)&idt;
    memset(&idt, 0, sizeof(idt));

    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00);

    idt_set_gate(0, isr0, 0x8E); idt_set_gate(1, isr1, 0x8E);
    idt_set_gate(2, isr2, 0x8E); idt_set_gate(3, isr3, 0x8E);
    idt_set_gate(4, isr4, 0x8E); idt_set_gate(5, isr5, 0x8E);
    idt_set_gate(6, isr6, 0x8E); idt_set_gate(7, isr7, 0x8E);
    idt_set_gate(8, isr8, 0x8E); idt_set_gate(9, isr9, 0x8E);
    idt_set_gate(10, isr10, 0x8E); idt_set_gate(11, isr11, 0x8E);
    idt_set_gate(12, isr12, 0x8E); idt_set_gate(13, isr13, 0x8E);
    idt_set_gate(14, isr14, 0x8E); idt_set_gate(15, isr15, 0x8E);
    idt_set_gate(16, isr16, 0x8E); idt_set_gate(17, isr17, 0x8E);
    idt_set_gate(18, isr18, 0x8E); idt_set_gate(19, isr19, 0x8E);
    idt_set_gate(20, isr20, 0x8E); idt_set_gate(21, isr21, 0x8E);
    idt_set_gate(22, isr22, 0x8E); idt_set_gate(23, isr23, 0x8E);
    idt_set_gate(24, isr24, 0x8E); idt_set_gate(25, isr25, 0x8E);
    idt_set_gate(26, isr26, 0x8E); idt_set_gate(27, isr27, 0x8E);
    idt_set_gate(28, isr28, 0x8E); idt_set_gate(29, isr29, 0x8E);
    idt_set_gate(30, isr30, 0x8E); idt_set_gate(31, isr31, 0x8E);

    idt_set_gate(32, irq0, 0x8E); idt_set_gate(33, irq1, 0x8E);
    idt_set_gate(34, irq2, 0x8E); idt_set_gate(35, irq3, 0x8E);
    idt_set_gate(36, irq4, 0x8E); idt_set_gate(37, irq5, 0x8E);
    idt_set_gate(38, irq6, 0x8E); idt_set_gate(39, irq7, 0x8E);
    idt_set_gate(40, irq8, 0x8E); idt_set_gate(41, irq9, 0x8E);
    idt_set_gate(42, irq10, 0x8E); idt_set_gate(43, irq11, 0x8E);
    idt_set_gate(44, irq12, 0x8E); idt_set_gate(45, irq13, 0x8E);
    idt_set_gate(46, irq14, 0x8E); idt_set_gate(47, irq15, 0x8E);

    asm volatile("lidt %0" : : "m"(idtp));
}

void exception_handler(struct registers* regs) {
    printf("\n!!! EXCEPTION !!!\nINT: %d (%s)\nERR: 0x%llX\n",
              regs->int_no, exception_messages[regs->int_no], regs->err_code);
    
    printf("RAX: 0x%llX RBX: 0x%llX\nRCX: 0x%llX RDX: 0x%llX\n",
              regs->rax, regs->rbx, regs->rcx, regs->rdx);
    printf("RSP: 0x%llX RBP: 0x%llX\nRIP: 0x%llX\n",
              regs->frame.rsp, regs->rbp, regs->frame.rip);
    
    if (regs->int_no != 3) asm volatile("cli; hlt");
}

void irq_handler(struct registers* regs) {
    uint8_t irq = regs->int_no - 32;
    if (irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](regs);
    }
    
    if (regs->int_no >= 40) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void register_irq_handler(uint8_t irq, void (*handler)(struct registers*)) {
    if (irq < 16) irq_handlers[irq] = handler;
}

void unregister_irq_handler(uint8_t irq) {
    if (irq < 16) irq_handlers[irq] = 0;
}