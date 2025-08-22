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
#include "fs/include/fat.h"
#include "../drivers/timer/timer.h"
#include <time.h>

extern int main();

void *map_vbe_framebuffer(uint32_t phys_fb_address, uint32_t width, uint32_t height, uint8_t bpp) {
    size_t fb_size = width * height * (bpp / 8);
    size_t page_count = (fb_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    uintptr_t virt_addr = 0xE0000000;
    
    if (map_pages(virt_addr, phys_fb_address, page_count, 
                 PAGE_PRESENT | PAGE_WRITABLE | PAGE_PWT | PAGE_PCD) != 0) {
        return NULL;
    }
    
    return (void *)virt_addr;
}

void draw_test_pattern(void *framebuffer, uint32_t width, uint32_t height, uint8_t bpp) {
    if (bpp == 32) {
        uint32_t *pixels = (uint32_t *)framebuffer;
        
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                uint8_t r = (x * 255) / width;
                uint8_t g = (y * 255) / height;
                uint8_t b = 128;
                pixels[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
}

void vbe_graphics_example(uint32_t magic, struct multiboot_info *mbi) {
    uint32_t phys_fb = get_framebuffer_address(magic, mbi);
    uint32_t width = 1024;
    uint32_t height = 768;
    uint8_t bpp = 32;
    
    if (phys_fb == 0) {
        return;
    }
    
    void *vbe_fb = map_vbe_framebuffer(phys_fb, width, height, bpp);
    if (!vbe_fb) {
        return;
    }
    
    if (bpp == 32) {
        uint32_t *pixels = (uint32_t *)vbe_fb;
        uint32_t blue_color = 0xFF0000FF;
        
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                pixels[y * width + x] = blue_color;
            }
        }
    }
    
    draw_test_pattern(vbe_fb, width, height, bpp);
}



void kmain(uint32_t magic, struct multiboot_info *mbi) {
    idt_init();
    init_timer(1000);
    init_keyboard();
    pci_init();
    dma_init();       
    asm volatile ("sti");

    if (ata_init() == 0) {
        printf("Disk Error\n");
    }

    ata_device_t *dev = get_ata_device(0);
    
    uintptr_t phys_mem_start = 0x100000;
    uintptr_t phys_mem_end = 0x2000000;
    
    mm_init(phys_mem_start, phys_mem_end);
    
    vbe_graphics_example(magic, mbi);

    for (;;);
}