ENTRY(_start)

KERNEL_BASE = 0x00100000;

SECTIONS {
    . = KERNEL_BASE;

    .multiboot : {
        *(.multiboot_header)
    } :multiboot

    .text ALIGN(4K) : {
        *(.text .text.*)
    } :text

    .rodata ALIGN(4K) : {
        *(.rodata*)
    } :rodata

    .data ALIGN(4K) : {
        *(.data .data.*)
    } :data

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss .bss.*)
        
        pml4_table = .;
        . += 4K;
        pdp_table = .;
        . += 4K;
        pd_table = .;
        . += 4K;

        stack_bottom = .;
        . += 16K;
        stack_top = .;
    } :bss

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}

PHDRS {
    multiboot PT_LOAD FLAGS((1 << 0));      
    text PT_LOAD FLAGS((1 << 0) | (1 << 2));
    rodata PT_LOAD FLAGS(1 << 0);           
    data PT_LOAD FLAGS((1 << 0) | (1 << 1));
    bss PT_LOAD FLAGS((1 << 0) | (1 << 1)); 
}