#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>
#include <stddef.h>

// Магическое число Multiboot2
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// Базовая структура тега
typedef struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} multiboot_tag_t;

// Тег с информацией о памяти (базовый)
typedef struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;  // KB памяти до 1MB
    uint32_t mem_upper;  // KB памяти после 1MB
} multiboot_tag_basic_meminfo_t;

// Тег с картой памяти
typedef struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} multiboot_tag_mmap_t;

// Запись в карте памяти
typedef struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed)) multiboot_mmap_entry_t;

// Тег с информацией о модулях
typedef struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[0];
} multiboot_tag_module_t;

// Тег с ELF секциями
typedef struct multiboot_tag_elf_sections {
    uint32_t type;
    uint32_t size;
    uint32_t num;
    uint32_t entsize;
    uint32_t shndx;
    char sections[0];
} multiboot_tag_elf_sections_t;

// Типы тегов Multiboot2
#define MULTIBOOT_TAG_TYPE_END                  0
#define MULTIBOOT_TAG_TYPE_CMDLINE              1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME     2
#define MULTIBOOT_TAG_TYPE_MODULE               3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO        4
#define MULTIBOOT_TAG_TYPE_BOOTDEV              5
#define MULTIBOOT_TAG_TYPE_MMAP                 6
#define MULTIBOOT_TAG_TYPE_VBE                  7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER          8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS         9

// Типы областей памяти
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5

// Выравнивание тегов
#define MULTIBOOT_TAG_ALIGN 8
#define MULTIBOOT_TAG_ALIGN_SIZE(size) (((size) + MULTIBOOT_TAG_ALIGN - 1) & ~(MULTIBOOT_TAG_ALIGN - 1))

#endif // MULTIBOOT2_H