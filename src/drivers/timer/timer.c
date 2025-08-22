#include "timer.h"
#include <asm/io.h>
#include "../../kernel/idt/idt.h"

#define PIT_FREQUENCY 1193180
#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40

volatile uint64_t timer_ticks = 0;

static void timer_callback(struct registers* regs) {
    (void)regs;
    timer_ticks++;
}

void init_timer(uint32_t frequency) {
    register_irq_handler(0, timer_callback);

    uint32_t divisor = PIT_FREQUENCY / frequency;
    outb(PIT_COMMAND_PORT, 0x36);
    outb(PIT_CHANNEL0_PORT, divisor & 0xFF);
    outb(PIT_CHANNEL0_PORT, (divisor >> 8) & 0xFF);
}

uint64_t get_timer_ticks() {
    return timer_ticks;
}

void Sleep(uint64_t milliseconds) {
    uint64_t end_ticks = timer_ticks + milliseconds;
    while (timer_ticks < end_ticks) {
        asm volatile("sti; hlt; cli");
    }

    asm volatile("sti");
}