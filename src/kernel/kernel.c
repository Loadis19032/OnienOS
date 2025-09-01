#include "stdint.h"
#include "stddef.h"
#include <stdio.h>
#include "idt/idt.h"
#include <asm/io.h>
#include <string.h>
#include <ctype.h>
#include "mm/mem.h"
#include "../drivers/keyboard/keyboard.h"
#include <math.h>
#include <stdlib.h>
#include "boot/multiboot2.h" 
#include <assert.h>
#include "../drivers/dma/dma.h"
#include "../drivers/pci/pci.h"
#include "../drivers/disk/pata/pata.h"
#include "fs/include/pros.h"
#include "../drivers/timer/timer.h"
#include <time.h>
#include "panic/panic.h"
#include "../drivers/mouse/mouse.h"
#include "../drivers/vbe/vbe.h"

extern void shell();
extern void _s();

void kmain() {
    idt_init();
    init_timer(100);
    init_keyboard();
    pci_init();
    dma_init();       
    asm volatile ("sti");

    ata_init();
    ata_device_t *hdd = get_ata_device(0);

    uintptr_t phys_mem_start = 0x100000;
    uintptr_t phys_mem_end = 32 * 1024 * 1024;
    
    mm_init(phys_mem_start, phys_mem_end); 

    mouse_init();
    //shell();

    //vga_init();
    //mouse_init();

    //while (1) {
    //    vga_clear_buffer(0);
    //    mouse_state_t mouse = mouse_get_state();
//
    //    //vga_draw_from_array(image_data, 0, 0, 639, 479);
//
    //    vga_fill_triangle(mouse.x, mouse.y, mouse.x, mouse.y + 15, mouse.x + 8, mouse.y + 10, VGA_WHITE);
//
    //    vga_swap_buffers();
    //}

    if (init_lfb() != 0) {
        return;
    }

    int frames = 0;
    time_t start_time = time(NULL);
    time_t current_time;
    char fps_text[32];

    while (1) {
        clear_screen(0xff000000);

        fill_rect(100, 100, 100, 100, rgba(255, 0, 0, 255)); 

        mouse_state_t mouse = mouse_get_state();
        fill_triangle(mouse.x, mouse.y, mouse.x, mouse.y + 16, mouse.x + 16, mouse.y + 16, 0xffffff);
        draw_line(mouse.x + 5, mouse.y + 16, mouse.x + 10, mouse.y + 16 + 8, 0xFFFFFFFF);

        frames++;
        current_time = time(NULL);

        if(current_time - start_time >= 1) {
            sprintf(fps_text, "FPS: %d", frames);
            frames = 0;
            start_time = current_time;
        }

        draw_string(10, 10, fps_text, 0xFFFFFFFF);
        swap_buffers();
    }

    for (;;);
}

