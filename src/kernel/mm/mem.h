#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

// Константы для управления памятью
#define PAGE_SIZE 4096
#define HUGE_PAGE_SIZE (2 * 1024 * 1024)  // 2MB huge pages
#define HEAP_START 0x200000               // Начало кучи после 2MB
#define MIN_BLOCK_SIZE 16                 // Минимальный размер блока
#define MAGIC_ALLOC 0xDEADBEEF           // Магическое число для проверки
#define MAGIC_FREE 0xFEEDFACE            // Магическое число для свободных блоков

// Флаги для страниц
#define PAGE_PRESENT 0x001
#define PAGE_WRITABLE 0x002
#define PAGE_USER 0x004
#define PAGE_WRITE_THROUGH 0x008
#define PAGE_CACHE_DISABLE 0x010
#define PAGE_ACCESSED 0x020
#define PAGE_DIRTY 0x040
#define PAGE_HUGE 0x080
#define PAGE_GLOBAL 0x100
#define PAGE_NO_EXECUTE 0x8000000000000000ULL

// Макросы для работы с адресами страниц
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDP_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr) (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr) (((addr) >> 12) & 0x1FF)
#define PAGE_ALIGN_DOWN(addr) ((uintptr_t)(addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr) (((uintptr_t)(addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Структура заголовка блока памяти
typedef struct mem_block {
    uint32_t magic;           // Магическое число для проверки целостности
    size_t size;              // Размер блока (без заголовка)
    struct mem_block* next;   // Следующий блок в списке
    struct mem_block* prev;   // Предыдущий блок в списке
    uint8_t is_free;          // 1 если блок свободен, 0 если занят
    uint8_t padding[3];       // Выравнивание до 8 байт
} __attribute__((packed)) mem_block_t;

// Статистика использования памяти
typedef struct mem_stats {
    size_t total_ram;         // Общий объем RAM
    size_t used_ram;          // Используемая память
    size_t free_ram;          // Свободная память
    size_t heap_size;         // Размер кучи
    size_t allocated_blocks;  // Количество выделенных блоков
    size_t free_blocks;       // Количество свободных блоков
} mem_stats_t;

// Основные функции управления памятью
void mem_init(size_t total_ram, void* heap_start_addr);
void* kmalloc(size_t size);
void kfree(void* ptr);
size_t getusedram(void);

// Дополнительные функции для отладки и статистики
void* kcalloc(size_t count, size_t size);
void* krealloc(void* ptr, size_t new_size);
mem_stats_t get_mem_stats(void);
void print_mem_info(void);
int mem_check_integrity(void);

// Функции для работы с виртуальной памятью и маппингом
int map_page(uint64_t virtual_addr, uint64_t physical_addr, uint32_t flags);
int unmap_page(uint64_t virtual_addr);
uint64_t get_physical_addr(uint64_t virtual_addr);
void* map_physical_memory(uint64_t physical_addr, size_t size, uint32_t flags);
void unmap_physical_memory(void* virtual_addr, size_t size);

// Внутренние функции (не для внешнего использования)
void merge_free_blocks(mem_block_t* block);
uint64_t get_pml4_table(void);

#endif // MEM_H