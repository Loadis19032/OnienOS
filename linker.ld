ENTRY(_start)

KERNEL_BASE = 0x00100000;

SECTIONS {
    . = KERNEL_BASE;

    #.multiboot : {
    #    *(.multiboot_header)
    #}

    .text ALIGN(4K) : {
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        
        pml4_table = .;
        . += 4K;
        pdp_table = .;
        . += 4K;
        pd_table = .;
        . += 4K;

        stack_bottom = .;
        . += 16K;
        stack_top = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}