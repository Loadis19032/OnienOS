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

extern void shell();
extern void _s();



void kmain(uint32_t magic, struct multiboot_info *mbi) {
    idt_init();
    init_timer(1000);
    init_keyboard();
    pci_init();
    dma_init();       
    asm volatile ("sti");

    ata_init();
    ata_device_t *hdd = get_ata_device(0);

    uintptr_t phys_mem_start = 0x100000;
    uintptr_t phys_mem_end = 32 * 1024 * 1024;
    
    mm_init(phys_mem_start, phys_mem_end); 
    
    //shell();

    //demo_basic_operations();
    //demo_file_operations_with_offset();
    //demo_open_file_operations();
    //demo_directory_operations();
    //demo_large_file_operations();
    //demo_error_handling();
    //
    //printf("All demos completed successfully!\n");

    _s();

    for (;;);
}
