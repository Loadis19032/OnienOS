MAGIC equ 0xE85250D6    
ARCHITECTURE equ 0      
HEADER_LENGTH equ header_end - header_start
CHECKSUM equ -(MAGIC + ARCHITECTURE + HEADER_LENGTH)

PAGE_SIZE equ 4096
PAGE_PRESENT equ 1
PAGE_WRITABLE equ 2
PAGE_HUGE equ 0x80

section .multiboot_header
align 8
header_start:
    dd MAGIC             
    dd ARCHITECTURE      
    dd HEADER_LENGTH     
    dd CHECKSUM          

    dw 0        
    dw 0        
    dd 8        
header_end:

section .bss
align 4096
pml4_table:
    resb PAGE_SIZE
pdp_table:
    resb PAGE_SIZE
pd_table:
    resb PAGE_SIZE

stack_bottom:
    resb 16384      
stack_top:

section .data
gdt64:
    dq 0           
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) 
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) 
.pointer:
    dw $ - gdt64 - 1             
    dq gdt64                     

error_cpuid_msg db 'ERROR: CPUID not supported', 0
error_long_mode_msg db 'ERROR: Long mode not supported', 0
error_success_msg db 'Successfully entered long mode!', 0

section .text
bits 32
global _start

_start:
    mov esp, stack_top
    
    push ebx
    push eax

    call check_cpuid
    test eax, eax
    jz .cpuid_error

    call check_long_mode
    test eax, eax
    jz .long_mode_error

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]

    jmp gdt64.code:long_mode_start

.multiboot_error:
    mov esi, error_cpuid_msg
    call print_string
    hlt

.cpuid_error:
    mov esi, error_cpuid_msg
    call print_string
    hlt

.long_mode_error:
    mov esi, error_long_mode_msg
    call print_string
    hlt

check_cpuid:
    pushfd                     
    pop eax                    
    mov ecx, eax               
    xor eax, 1 << 21           
    push eax                   
    popfd                      
    pushfd                     
    pop eax                    
    push ecx                   
    popfd
    
    cmp eax, ecx
    je .no_cpuid
    mov eax, 1        
    ret
.no_cpuid:
    mov eax, 0   
    ret

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29    
    jz .no_long_mode
    
    mov eax, 1          
    ret
.no_long_mode:
    mov eax, 0     
    ret

setup_page_tables:
    mov edi, pml4_table
    mov ecx, 3 * PAGE_SIZE / 4
    xor eax, eax
    rep stosd
    
    mov eax, pdp_table
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [pml4_table], eax
    
    mov eax, pd_table
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [pdp_table], eax
    
    mov edi, pd_table
    mov eax, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE
    mov ecx, 512             
.setup_pd_loop:
    mov [edi], eax
    add eax, 0x200000           
    add edi, 8                  
    loop .setup_pd_loop
    
    ret

enable_paging:
    mov eax, pml4_table
    mov cr3, eax
    
    mov eax, cr4
    or eax, 1 << 5     
    mov cr4, eax
    
    mov ecx, 0xC0000080      
    rdmsr
    or eax, 1 << 8      
    wrmsr
    
    mov eax, cr0
    or eax, 1 << 31 
    mov cr0, eax
    
    ret

print_string:
    push eax
    push ebx
    mov ebx, 0xB8000      
    mov ah, 0x0F          
.print_loop:
    lodsb
    test al, al
    jz .print_done
    mov [ebx], ax
    add ebx, 2
    jmp .print_loop
.print_done:
    pop ebx
    pop eax
    ret

bits 64
extern kmain
long_mode_start:
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov rsp, stack_top
    
    cld
    
    call kmain
    
    cli
.halt_loop:
    hlt
    jmp .halt_loop

section .rodata
success_message db 'Long mode kernel running successfully!', 0