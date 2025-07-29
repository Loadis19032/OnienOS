#include "mem.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Глобальные переменные для управления памятью
static mem_block_t* heap_start = NULL;
static mem_block_t* heap_end = NULL;
static size_t total_ram = 0;
static size_t used_ram = 0;
static uint64_t* pml4_table = NULL;
static size_t total_pages = 0;
static size_t used_pages = 0;
static size_t mapped_pages = 0;

// Внутренние функции
static void update_page_stats(void);
static mem_block_t* coalesce_blocks(mem_block_t* block);
static void* allocate_large_block(size_t size);

// Инициализация менеджера памяти
void mem_init(size_t total_ram_size, void* heap_start_addr) {
    total_ram = total_ram_size;
    used_ram = 0;
    total_pages = total_ram / PAGE_SIZE;
    used_pages = 0;
    
    // Инициализация кучи
    heap_start = (mem_block_t*)heap_start_addr;
    heap_start->magic = MAGIC_FREE;
    heap_start->size = total_ram - sizeof(mem_block_t);
    heap_start->next = NULL;
    heap_start->prev = NULL;
    heap_start->is_free = 1;
    
    heap_end = heap_start;
    
    // Сохраняем указатель на таблицу PML4
    pml4_table = (uint64_t*)get_pml4_table();
    
    update_page_stats();
}

// Выравнивание размера блока
static size_t align_size(size_t size) {
    if (size < MIN_BLOCK_SIZE) {
        return MIN_BLOCK_SIZE;
    }
    
    // Для больших блоков выравниваем по границе страницы
    if (size >= PAGE_SIZE) {
        return PAGE_ALIGN_UP(size);
    }
    
    // Для маленьких блоков выравниваем до 8 байт
    return (size + 7) & ~7;
}

// Поиск свободного блока (best-fit алгоритм)
static mem_block_t* find_free_block(size_t size) {
    mem_block_t* best = NULL;
    mem_block_t* current = heap_start;
    size_t best_diff = SIZE_MAX;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            // Точное соответствие - сразу возвращаем
            if (current->size == size) {
                return current;
            }
            
            // Ищем блок с наименьшей разницей в размере
            size_t diff = current->size - size;
            if (diff < best_diff) {
                best = current;
                best_diff = diff;
            }
        }
        current = current->next;
    }
    
    return best;
}

// Разделение блока
static mem_block_t* split_block(mem_block_t* block, size_t size) {
    size_t remaining = block->size - size;
    
    // Не разделяем если остаток слишком мал
    if (remaining < sizeof(mem_block_t) + MIN_BLOCK_SIZE) {
        return block;
    }
    
    // Создаем новый блок из оставшегося пространства
    mem_block_t* new_block = (mem_block_t*)((uint8_t*)block + sizeof(mem_block_t) + size);
    new_block->magic = MAGIC_FREE;
    new_block->size = remaining - sizeof(mem_block_t);
    new_block->is_free = 1;
    new_block->prev = block;
    new_block->next = block->next;
    
    if (block->next) {
        block->next->prev = new_block;
    }
    block->next = new_block;
    
    if (block == heap_end) {
        heap_end = new_block;
    }
    
    // Обновляем текущий блок
    block->size = size;
    
    return block;
}

// Объединение свободных блоков
static mem_block_t* coalesce_blocks(mem_block_t* block) {
    // Объединяем с предыдущим блоком
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size + sizeof(mem_block_t);
        block->prev->next = block->next;
        
        if (block->next) {
            block->next->prev = block->prev;
        }
        
        if (block == heap_end) {
            heap_end = block->prev;
        }
        
        block = block->prev;
    }
    
    // Объединяем со следующим блоком
    if (block->next && block->next->is_free) {
        block->size += block->next->size + sizeof(mem_block_t);
        block->next = block->next->next;
        
        if (block->next) {
            block->next->prev = block;
        }
        
        if (block->next == NULL) {
            heap_end = block;
        }
    }
    
    return block;
}

// Выделение большого блока (выровненного по странице)
static void* allocate_large_block(size_t size) {
    // Ищем свободный блок достаточного размера
    mem_block_t* block = find_free_block(size);
    if (!block) {
        return NULL;
    }
    
    // Выравниваем адрес начала блока по границе страницы
    uint8_t* aligned_start = (uint8_t*)PAGE_ALIGN_UP((uintptr_t)block + sizeof(mem_block_t));
    size_t aligned_size = PAGE_ALIGN_UP(size);
    
    // Проверяем, можно ли создать новый блок перед выровненной областью
    if ((uint8_t*)block + sizeof(mem_block_t) < aligned_start) {
        size_t prefix_size = aligned_start - (uint8_t*)block - sizeof(mem_block_t);
        
        // Если есть место для блока перед выровненной областью
        if (prefix_size >= sizeof(mem_block_t) + MIN_BLOCK_SIZE) {
            mem_block_t* prefix_block = block;
            prefix_block->size = prefix_size;
            
            // Создаем выровненный блок
            mem_block_t* aligned_block = (mem_block_t*)(aligned_start - sizeof(mem_block_t));
            aligned_block->magic = MAGIC_ALLOC;
            aligned_block->size = aligned_size;
            aligned_block->is_free = 0;
            aligned_block->prev = prefix_block;
            aligned_block->next = block->next;
            
            prefix_block->next = aligned_block;
            
            if (aligned_block->next) {
                aligned_block->next->prev = aligned_block;
            }
            
            if (block == heap_end) {
                heap_end = aligned_block;
            }
            
            // Создаем блок после выровненной области, если есть место
            uint8_t* block_end = aligned_start + aligned_size;
            uint8_t* next_block_start = (uint8_t*)aligned_block + sizeof(mem_block_t) + aligned_size;
            
            if (block_end < next_block_start) {
                size_t suffix_size = next_block_start - block_end;
                
                if (suffix_size >= sizeof(mem_block_t) + MIN_BLOCK_SIZE) {
                    mem_block_t* suffix_block = (mem_block_t*)block_end;
                    suffix_block->magic = MAGIC_FREE;
                    suffix_block->size = suffix_size - sizeof(mem_block_t);
                    suffix_block->is_free = 1;
                    suffix_block->prev = aligned_block;
                    suffix_block->next = aligned_block->next;
                    
                    aligned_block->next = suffix_block;
                    
                    if (suffix_block->next) {
                        suffix_block->next->prev = suffix_block;
                    }
                    
                    if (aligned_block == heap_end) {
                        heap_end = suffix_block;
                    }
                }
            }
            
            used_ram += aligned_block->size + sizeof(mem_block_t);
            update_page_stats();
            return aligned_start;
        }
    }
    
    // Если не удалось выровнять, просто выделяем обычный блок
    block = split_block(block, size);
    block->is_free = 0;
    block->magic = MAGIC_ALLOC;
    
    used_ram += block->size + sizeof(mem_block_t);
    update_page_stats();
    
    return (void*)((uint8_t*)block + sizeof(mem_block_t));
}

// Выделение памяти
void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    size = align_size(size);
    
    // Для больших блоков используем специальный алгоритм
    if (size >= PAGE_SIZE) {
        return allocate_large_block(size);
    }
    
    mem_block_t* block = find_free_block(size);
    if (!block) {
        return NULL;
    }
    
    block = split_block(block, size);
    block->is_free = 0;
    block->magic = MAGIC_ALLOC;
    
    used_ram += block->size + sizeof(mem_block_t);
    update_page_stats();
    
    return (void*)((uint8_t*)block + sizeof(mem_block_t));
}

// Освобождение памяти
void kfree(void* ptr) {
    if (!ptr) return;
    
    mem_block_t* block = (mem_block_t*)((uint8_t*)ptr - sizeof(mem_block_t));
    
    if (block->magic != MAGIC_ALLOC) {
        return; // Неверный указатель или двойное освобождение
    }
    
    block->magic = MAGIC_FREE;
    block->is_free = 1;
    used_ram -= block->size + sizeof(mem_block_t);
    
    coalesce_blocks(block);
    update_page_stats();
}

// Выделение и обнуление памяти
void* kcalloc(size_t count, size_t size) {
    size_t total_size = count * size;
    void* ptr = kmalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// Изменение размера блока памяти
void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    mem_block_t* block = (mem_block_t*)((uint8_t*)ptr - sizeof(mem_block_t));
    if (block->magic != MAGIC_ALLOC) {
        return NULL;
    }
    
    new_size = align_size(new_size);
    
    // Если новый размер меньше или равен текущему
    if (new_size <= block->size) {
        // Пытаемся разделить блок, если осталось достаточно места
        if (block->size - new_size >= sizeof(mem_block_t) + MIN_BLOCK_SIZE) {
            split_block(block, new_size);
        }
        return ptr;
    }
    
    // Проверяем, можно ли расшириться в соседний свободный блок
    if (block->next && block->next->is_free && 
        (block->size + sizeof(mem_block_t) + block->next->size) >= new_size) {
        size_t needed = new_size - block->size;
        
        // Объединяем блоки
        block->size += block->next->size + sizeof(mem_block_t);
        block->next = block->next->next;
        
        if (block->next) {
            block->next->prev = block;
        }
        
        if (block->next == NULL) {
            heap_end = block;
        }
        
        // Разделяем, если осталось место
        if (block->size > new_size + sizeof(mem_block_t) + MIN_BLOCK_SIZE) {
            split_block(block, new_size);
        }
        
        used_ram += (new_size - (block->size - sizeof(mem_block_t)));
        update_page_stats();
        return ptr;
    }
    
    // Иначе выделяем новый блок и копируем данные
    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}

// Обновление статистики страниц
static void update_page_stats() {
    size_t heap_pages = 0;
    mem_block_t* current = heap_start;
    
    while (current) {
        if (!current->is_free) {
            uintptr_t start = (uintptr_t)current;
            uintptr_t end = start + sizeof(mem_block_t) + current->size;
            uintptr_t aligned_start = PAGE_ALIGN_DOWN(start);
            uintptr_t aligned_end = PAGE_ALIGN_UP(end);
            heap_pages += (aligned_end - aligned_start) / PAGE_SIZE;
        }
        current = current->next;
    }
    
    used_pages = heap_pages + mapped_pages;
}

size_t getusedram() {
    return (used_pages + mapped_pages) * (PAGE_SIZE / 1024); // в KB
}

// Получение статистики памяти
mem_stats_t get_mem_stats() {
    mem_stats_t stats = {0};
    stats.total_ram = total_ram;
    stats.used_ram = used_pages * PAGE_SIZE;
    stats.free_ram = total_ram - stats.used_ram;
    
    // Подсчёт блоков
    mem_block_t* curr = heap_start;
    while (curr) {
        if (curr->is_free) stats.free_blocks++;
        else stats.allocated_blocks++;
        curr = curr->next;
    }
    
    return stats;
}

// Проверка целостности кучи
int mem_check_integrity(void) {
    mem_block_t* current = heap_start;
    while (current) {
        // Проверка магических чисел
        if ((current->is_free && current->magic != MAGIC_FREE) ||
            (!current->is_free && current->magic != MAGIC_ALLOC)) {
            return -1;
        }
        
        // Проверка связности списка
        if (current->next && current->next->prev != current) {
            return -1;
        }
        
        // Проверка перекрытия блоков
        if (current->next && 
            (uint8_t*)current + sizeof(mem_block_t) + current->size > (uint8_t*)current->next) {
            return -1;
        }
        
        current = current->next;
    }
    return 0;
}

// Функции для работы с виртуальной памятью
int map_page(uint64_t virtual_addr, uint64_t physical_addr, uint32_t flags) {
    uint64_t pml4_index = PML4_INDEX(virtual_addr);
    uint64_t pdp_index = PDP_INDEX(virtual_addr);
    uint64_t pd_index = PD_INDEX(virtual_addr);
    uint64_t pt_index = PT_INDEX(virtual_addr);

    // 1. Получаем указатель на PML4
    uint64_t* pml4 = (uint64_t*)get_pml4_table();
    
    // 2. Обрабатываем запись в PML4
    if (!(pml4[pml4_index] & PAGE_PRESENT)) {
        // Выделяем новую таблицу PDP (должно быть 4KB выровнено)
        uint64_t* pdp = (uint64_t*)kmalloc(PAGE_SIZE);
        if (!pdp) return -1; // Ошибка выделения
        memset(pdp, 0, PAGE_SIZE);
        
        pml4[pml4_index] = ((uint64_t)pdp) | flags | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    uint64_t* pdp = (uint64_t*)(pml4[pml4_index] & ~0xFFF);
    
    // 3. Обрабатываем запись в PDP
    if (!(pdp[pdp_index] & PAGE_PRESENT)) {
        uint64_t* pd = (uint64_t*)kmalloc(PAGE_SIZE);
        if (!pd) return -1;
        memset(pd, 0, PAGE_SIZE);
        
        pdp[pdp_index] = ((uint64_t)pd) | flags | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    uint64_t* pd = (uint64_t*)(pdp[pdp_index] & ~0xFFF);
    
    // 4. Обрабатываем запись в PD
    if (!(pd[pd_index] & PAGE_PRESENT)) {
        uint64_t* pt = (uint64_t*)kmalloc(PAGE_SIZE);
        if (!pt) return -1;
        memset(pt, 0, PAGE_SIZE);
        
        pd[pd_index] = ((uint64_t)pt) | flags | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    uint64_t* pt = (uint64_t*)(pd[pd_index] & ~0xFFF);
    
    // 5. Устанавливаем конечную запись
    pt[pt_index] = physical_addr | flags | PAGE_PRESENT;
    
    // Инвалидация TLB
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    mapped_pages++;
    update_page_stats();

    return 0;
}

int unmap_page(uint64_t virtual_addr) {
    uint64_t pml4_index = PML4_INDEX(virtual_addr);
    uint64_t pdp_index = PDP_INDEX(virtual_addr);
    uint64_t pd_index = PD_INDEX(virtual_addr);
    uint64_t pt_index = PT_INDEX(virtual_addr);

    uint64_t* pml4 = (uint64_t*)get_pml4_table();
    if (!(pml4[pml4_index] & PAGE_PRESENT)) return -1;

    uint64_t* pdp = (uint64_t*)(pml4[pml4_index] & ~0xFFF);
    if (!(pdp[pdp_index] & PAGE_PRESENT)) return -1;

    uint64_t* pd = (uint64_t*)(pdp[pdp_index] & ~0xFFF);
    if (!(pd[pd_index] & PAGE_PRESENT)) return -1;

    uint64_t* pt = (uint64_t*)(pd[pd_index] & ~0xFFF);
    if (!(pt[pt_index] & PAGE_PRESENT)) return -1;

    pt[pt_index] = 0;
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    if (mapped_pages > 0) mapped_pages--;
    update_page_stats();
    
    return 0;
}

uint64_t get_physical_addr(uint64_t virtual_addr) {
    uint64_t pml4_index = PML4_INDEX(virtual_addr);
    uint64_t pdp_index = PDP_INDEX(virtual_addr);
    uint64_t pd_index = PD_INDEX(virtual_addr);
    uint64_t pt_index = PT_INDEX(virtual_addr);
    
    if (!(pml4_table[pml4_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    uint64_t* pdp_table = (uint64_t*)(pml4_table[pml4_index] & ~0xFFF);
    if (!(pdp_table[pdp_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    uint64_t* pd_table = (uint64_t*)(pdp_table[pdp_index] & ~0xFFF);
    if (!(pd_table[pd_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    uint64_t* pt_table = (uint64_t*)(pd_table[pd_index] & ~0xFFF);
    if (!(pt_table[pt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    return (pt_table[pt_index] & ~0xFFF) | (virtual_addr & 0xFFF);
}

void* map_physical_memory(uint64_t phys_addr, size_t size, uint32_t flags) {
    // Выделяем виртуальный диапазон через kmalloc
    void* virt = kmalloc(size); 
    if (!virt) return NULL;
    
    // Постраничный маппинг
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        if (map_page((uint64_t)virt + i, phys_addr + i, flags) != 0) {
            // Откат при ошибке
            for (size_t j = 0; j < i; j += PAGE_SIZE) {
                unmap_page((uint64_t)virt + j);
            }
            kfree(virt);
            return NULL;
        }
    }
    return virt;
}

void unmap_physical_memory(void* virtual_addr, size_t size) {
    size = PAGE_ALIGN_UP(size);
    uint64_t va = (uint64_t)virtual_addr;
    
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        unmap_page(va + i);
    }
    
    kfree(virtual_addr);
}

uint64_t get_pml4_table(void) {
    uint64_t cr3;
    // Читаем значение регистра CR3 с помощью встроенной ассемблерной вставки
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & ~0xFFF; // Очищаем флаги, оставляем только физический адрес
}