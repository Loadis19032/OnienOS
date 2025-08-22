#include "mem.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"
#include "stddef.h"

static mm_struct mm = {0};

// ================= Buddy Allocator =================

static void buddy_init(buddy_t *buddy, uintptr_t base, size_t size) {
    memset(buddy, 0, sizeof(*buddy));
    buddy->base = base;
    buddy->size = size;
    buddy->nr_free = size / PAGE_SIZE;
    
    for (int i = 0; i <= MAX_ORDER; i++) {
        INIT_LIST_HEAD(&buddy->free_area[i]);
    }
    
    int order = MAX_ORDER;
    while (order >= 0) {
        size_t block_size = PAGE_SIZE << order;
        if (size >= block_size) {
            page_t *page = (page_t *)base;
            page->order = order;
            page->refcount = 0;
            page->flags = 0;
            list_add(&page->list, &buddy->free_area[order]);
            base += block_size;
            size -= block_size;
        } else {
            order--;
        }
    }
}

static void *buddy_alloc(buddy_t *buddy, int order) {
    if (order > MAX_ORDER || order < 0)
        return NULL;

    int current_order = order;
    while (current_order <= MAX_ORDER && 
           list_empty(&buddy->free_area[current_order])) {
        current_order++;
    }
    
    if (current_order > MAX_ORDER)
        return NULL;
    
    list_head_t *list = buddy->free_area[current_order].next;
    list_del(list);
    page_t *page = container_of(list, page_t, list);
    
    while (current_order > order) {
        current_order--;
        page_t *buddy_page = (page_t *)((uintptr_t)page + (PAGE_SIZE << current_order));
        
        buddy_page->order = current_order;
        buddy_page->refcount = 0;
        buddy_page->flags = 0;
        list_add(&buddy_page->list, &buddy->free_area[current_order]);
    }
    
    page->order = order;
    page->refcount = 1;
    buddy->nr_free -= (1 << order);
    return (void *)page;
}

static void buddy_free(buddy_t *buddy, void *addr, int order) {
    if (!addr || order > MAX_ORDER || order < 0)
        return;
        
    page_t *page = (page_t *)addr;
    uintptr_t page_addr = (uintptr_t)page;
    
    while (order < MAX_ORDER) {
        uintptr_t buddy_addr = page_addr ^ (PAGE_SIZE << order);
        page_t *buddy_page = (page_t *)buddy_addr;
        
        if ((int)buddy_page->order != order || buddy_page->refcount != 0)
            break;
            
        list_head_t *pos, *n;
        bool found = false;
        
        list_for_each_safe(pos, n, &buddy->free_area[order]) {
            page_t *p = container_of(pos, page_t, list);
            if (p == buddy_page) {
                list_del(pos);
                found = true;
                break;
            }
        }
        
        if (!found)
            break;
            
        if (page_addr > buddy_addr)
            page = buddy_page;
            
        page_addr &= ~((PAGE_SIZE << order) - 1);
        order++;
    }
    
    page->order = order;
    page->refcount = 0;
    list_add(&page->list, &buddy->free_area[order]);
    buddy->nr_free += (1 << order);
}

// ================= Slab Allocator =================

static slab_t *kmem_cache_grow(kmem_cache_t *cache) {
    slab_t *slab = (slab_t *)page_alloc(cache->order);
    if (!slab)
        return NULL;
        
    INIT_LIST_HEAD(&slab->list);
    slab->freelist = (void *)(slab + 1);
    slab->inuse = 0;
    slab->free = cache->objs_per_slab;
    slab->cache = cache;
    
    char *p = (char *)(slab + 1);
    for (unsigned int i = 0; i < cache->objs_per_slab - 1; i++) {
        *(void **)(p + i * cache->obj_size) = p + (i + 1) * cache->obj_size;
    }
    *(void **)(p + (cache->objs_per_slab - 1) * cache->obj_size) = NULL;
    
    cache->pages += (1 << cache->order);
    cache->objects += cache->objs_per_slab;
    
    return slab;
}

static void kmem_cache_init(kmem_cache_t *cache, const char *name, size_t size) {
    strncpy(cache->name, name, sizeof(cache->name)-1);
    cache->obj_size = ALIGN_UP(size, sizeof(void *));
    cache->order = 0;
    
    while ((size_t)(PAGE_SIZE << cache->order) < cache->obj_size * 16 && 
           cache->order < (unsigned int)MAX_ORDER) {
        cache->order++;
    }
    
    cache->objs_per_slab = (PAGE_SIZE << cache->order) / cache->obj_size;
    INIT_LIST_HEAD(&cache->slabs_full);
    INIT_LIST_HEAD(&cache->slabs_partial);
    INIT_LIST_HEAD(&cache->slabs_free);
    cache->objects = 0;
    cache->pages = 0;
}

void *kmem_cache_alloc(kmem_cache_t *cache) {
    slab_t *slab = NULL;
    
    if (!list_empty(&cache->slabs_partial)) {
        slab = list_first_entry(&cache->slabs_partial, slab_t, list);
    } else if (!list_empty(&cache->slabs_free)) {
        slab = list_first_entry(&cache->slabs_free, slab_t, list);
        list_del(&slab->list);
        list_add(&slab->list, &cache->slabs_partial);
    } else {
        slab = kmem_cache_grow(cache);
        if (!slab)
            return NULL;
        list_add(&slab->list, &cache->slabs_partial);
    }
    
    void *obj = slab->freelist;
    slab->freelist = *(void **)obj;
    slab->inuse++;
    slab->free--;
    
    if (slab->free == 0) {
        list_del(&slab->list);
        list_add(&slab->list, &cache->slabs_full);
    }
    
    return obj;
}

void kmem_cache_free(kmem_cache_t *cache, void *obj) {
    if (!obj || !cache)
        return;
        
    slab_t *slab = (slab_t *)((uintptr_t)obj & PAGE_MASK);
    
    if (slab->cache != cache) {
        printf("kmem_cache_free: Wrong cache for object %p\n", obj);
        return;
    }
    
    *(void **)obj = slab->freelist;
    slab->freelist = obj;
    slab->inuse--;
    slab->free++;
    
    if (slab->inuse == 0) {
        list_del(&slab->list);
        list_add(&slab->list, &cache->slabs_free);
    } else if (slab->inuse == cache->objs_per_slab - 1) {
        list_del(&slab->list);
        list_add(&slab->list, &cache->slabs_partial);
    }
}

kmem_cache_t *kmem_cache_create(const char *name, size_t size) {
    if (size > SLAB_MAX_SIZE || size < SLAB_MIN_SIZE)
        return NULL;
        
    kmem_cache_t *cache = (kmem_cache_t *)kmalloc(sizeof(kmem_cache_t));
    if (!cache)
        return NULL;
        
    kmem_cache_init(cache, name, size);
    return cache;
}

void kmem_cache_destroy(kmem_cache_t *cache) {
    if (!cache)
        return;
        
    slab_t *slab, *tmp;
    list_for_each_entry_safe(slab, tmp, &cache->slabs_free, list) {
        page_free(slab, cache->order);
    }
    list_for_each_entry_safe(slab, tmp, &cache->slabs_partial, list) {
        page_free(slab, cache->order);
    }
    list_for_each_entry_safe(slab, tmp, &cache->slabs_full, list) {
        page_free(slab, cache->order);
    }
    
    kfree(cache);
}

// ================= Memory Manager =================

void mm_init(uintptr_t phys_mem_start, uintptr_t phys_mem_end) {
    if (mm.initialized)
        return;
    
    mm.phys_offset = 0;
    uintptr_t virt_start = phys_to_virt(phys_mem_start);
    uintptr_t virt_end = phys_to_virt(phys_mem_end);
    
    virt_start = ALIGN_UP(virt_start, PAGE_SIZE);
    virt_end = ALIGN_DOWN(virt_end, PAGE_SIZE);
    
    if (virt_end <= virt_start) {
        printf("Error: Invalid memory range: 0x%x - 0x%x\n", virt_start, virt_end);
        return;
    }
    
    buddy_init(&mm.buddy, virt_start, virt_end - virt_start);
    
    const char *names[] = {
        "kmalloc-16", "kmalloc-32", "kmalloc-64", "kmalloc-128",
        "kmalloc-256", "kmalloc-512", "kmalloc-1024", "kmalloc-2048"
    };
    size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    
    for (int i = 0; i < 8; i++) {
        kmem_cache_init(&mm.kmalloc_caches[i], names[i], sizes[i]);
    }
    
    mm.initialized = true;
    printf("Memory manager initialized: %u KB available\n", 
          (virt_end - virt_start) / 1024);
}

void *kmalloc(size_t size) {
    if (!mm.initialized)
        return NULL;
        
    if (size <= SLAB_MAX_SIZE) {
        for (int i = 0; i < 8; i++) {
            if (size <= mm.kmalloc_caches[i].obj_size) {
                return kmem_cache_alloc(&mm.kmalloc_caches[i]);
            }
        }
    }
    
    int order = 0;
    while ((size_t)(PAGE_SIZE << order) < size && order < MAX_ORDER)
        order++;
    
    return page_alloc(order);
}

void *kcalloc(size_t n, size_t size) {
    void *ptr = kmalloc(n * size);
    if (ptr)
        memset(ptr, 0, n * size);
    return ptr;
}

void *krealloc(void *p, size_t size) {
    if (!p)
        return kmalloc(size);
        
    if (size == 0) {
        kfree(p);
        return NULL;
    }
    
    size_t old_size = kmalloc_size(p);
    void *new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, p, old_size < size ? old_size : size);
        kfree(p);
    }
    return new_ptr;
}

void kfree(const void *ptr) {
    if (!ptr || !mm.initialized)
        return;
        
    for (int i = 0; i < 8; i++) {
        kmem_cache_t *cache = &mm.kmalloc_caches[i];
        slab_t *slab;
        
        list_for_each_entry(slab, &cache->slabs_full, list) {
            if ((uintptr_t)ptr >= (uintptr_t)(slab + 1) && 
                (uintptr_t)ptr < (uintptr_t)(slab + 1) + (PAGE_SIZE << cache->order)) {
                kmem_cache_free(cache, (void *)ptr);
                return;
            }
        }
        
        list_for_each_entry(slab, &cache->slabs_partial, list) {
            if ((uintptr_t)ptr >= (uintptr_t)(slab + 1) && 
                (uintptr_t)ptr < (uintptr_t)(slab + 1) + (PAGE_SIZE << cache->order)) {
                kmem_cache_free(cache, (void *)ptr);
                return;
            }
        }
    }
    
    page_t *page = (page_t *)((uintptr_t)ptr & PAGE_MASK);
    page_free(page, page->order);
}

void *page_alloc(int order) {
    if (!mm.initialized || order > MAX_ORDER || order < 0)
        return NULL;
        
    return buddy_alloc(&mm.buddy, order);
}

void page_free(void *addr, int order) {
    if (!addr || !mm.initialized || order > MAX_ORDER || order < 0)
        return;
        
    buddy_free(&mm.buddy, addr, order);
}

// ================= Virtual Memory =================

// Вспомогательная функция для проверки пустых таблиц
static bool is_table_empty(uint64_t *table) {
    for (int i = 0; i < 512; i++) {
        if (table[i] & PAGE_PRESENT) {
            return false;
        }
    }
    return true;
}

int map_pages(uintptr_t virt, uintptr_t phys, size_t count, uint64_t flags) {
    if (!mm.initialized)
        return -1;
    
    uint64_t *pml4 = (uint64_t *)phys_to_virt(PML4_BASE);
    
    for (size_t i = 0; i < count; i++) {
        uint64_t *pdp, *pd, *pt;
        uintptr_t cur_virt = virt + i * PAGE_SIZE;
        uintptr_t cur_phys = phys + i * PAGE_SIZE;

        // PML4
        uint64_t pml4e = pml4[PML4_INDEX(cur_virt)];
        if (!(pml4e & PAGE_PRESENT)) {
            void *pdp_phys = page_alloc(0);
            if (!pdp_phys) {
                // Откатываем все предыдущие маппинги
                unmap_pages(virt, i);
                return -1;
            }
            pdp = (uint64_t *)phys_to_virt((uintptr_t)pdp_phys);
            memset(pdp, 0, PAGE_SIZE);
            pml4[PML4_INDEX(cur_virt)] = (uintptr_t)pdp_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        } else {
            pdp = (uint64_t *)phys_to_virt(pml4e & PAGE_MASK);
        }

        // PDP
        uint64_t pdpe = pdp[PDP_INDEX(cur_virt)];
        if (!(pdpe & PAGE_PRESENT)) {
            void *pd_phys = page_alloc(0);
            if (!pd_phys) {
                unmap_pages(virt, i);
                return -1;
            }
            pd = (uint64_t *)phys_to_virt((uintptr_t)pd_phys);
            memset(pd, 0, PAGE_SIZE);
            pdp[PDP_INDEX(cur_virt)] = (uintptr_t)pd_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        } else {
            pd = (uint64_t *)phys_to_virt(pdpe & PAGE_MASK);
        }

        // PD
        uint64_t pde = pd[PD_INDEX(cur_virt)];
        if (!(pde & PAGE_PRESENT)) {
            void *pt_phys = page_alloc(0);
            if (!pt_phys) {
                unmap_pages(virt, i);
                return -1;
            }
            pt = (uint64_t *)phys_to_virt((uintptr_t)pt_phys);
            memset(pt, 0, PAGE_SIZE);
            pd[PD_INDEX(cur_virt)] = (uintptr_t)pt_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        } else {
            pt = (uint64_t *)phys_to_virt(pde & PAGE_MASK);
        }

        // PT
        pt[PT_INDEX(cur_virt)] = cur_phys | flags;
        asm volatile("invlpg (%0)" : : "r" (cur_virt) : "memory");
    }
    
    return 0;
}

void unmap_pages(uintptr_t virt, size_t count) {
    if (!mm.initialized)
        return;
        
    uint64_t *pml4 = (uint64_t *)phys_to_virt(PML4_BASE);
    
    for (size_t i = 0; i < count; i++) {
        uintptr_t cur_virt = virt + i * PAGE_SIZE;
        
        uint64_t pml4e = pml4[PML4_INDEX(cur_virt)];
        if (!(pml4e & PAGE_PRESENT))
            continue;
            
        uint64_t *pdp = (uint64_t *)phys_to_virt(pml4e & PAGE_MASK);
        uint64_t pdpe = pdp[PDP_INDEX(cur_virt)];
        if (!(pdpe & PAGE_PRESENT))
            continue;
            
        uint64_t *pd = (uint64_t *)phys_to_virt(pdpe & PAGE_MASK);
        uint64_t pde = pd[PD_INDEX(cur_virt)];
        if (!(pde & PAGE_PRESENT))
            continue;
            
        uint64_t *pt = (uint64_t *)phys_to_virt(pde & PAGE_MASK);
        pt[PT_INDEX(cur_virt)] = 0;
        
        // Проверяем и освобождаем пустые таблицы
        if (is_table_empty(pt)) {
            page_free((void *)(pde & PAGE_MASK), 0);
            pd[PD_INDEX(cur_virt)] = 0;
        }
        
        if (is_table_empty(pd)) {
            page_free((void *)(pdpe & PAGE_MASK), 0);
            pdp[PDP_INDEX(cur_virt)] = 0;
        }
        
        if (is_table_empty(pdp)) {
            page_free((void *)(pml4e & PAGE_MASK), 0);
            pml4[PML4_INDEX(cur_virt)] = 0;
        }
        
        asm volatile("invlpg (%0)" : : "r" (cur_virt) : "memory");
    }
}

uintptr_t virt_to_phys(uintptr_t virt) {
    if (!mm.initialized)
        return 0;
        
    uint64_t *pml4 = (uint64_t *)phys_to_virt(PML4_BASE);
    uint64_t pml4e = pml4[PML4_INDEX(virt)];
    if (!(pml4e & PAGE_PRESENT))
        return 0;
        
    uint64_t *pdp = (uint64_t *)phys_to_virt(pml4e & PAGE_MASK);
    uint64_t pdpe = pdp[PDP_INDEX(virt)];
    
    // Проверка на 1GB страницы
    if (pdpe & PAGE_HUGE) {
        return (pdpe & ~0x3FFFFF) | (virt & 0x3FFFFF);
    }
    
    if (!(pdpe & PAGE_PRESENT))
        return 0;
        
    uint64_t *pd = (uint64_t *)phys_to_virt(pdpe & PAGE_MASK);
    uint64_t pde = pd[PD_INDEX(virt)];
    
    // Проверка на 2MB страницы
    if (pde & PAGE_HUGE) {
        return (pde & ~0x1FFFFF) | (virt & 0x1FFFFF);
    }
    
    if (!(pde & PAGE_PRESENT))
        return 0;
        
    uint64_t *pt = (uint64_t *)phys_to_virt(pde & PAGE_MASK);
    uint64_t pte = pt[PT_INDEX(virt)];
    if (!(pte & PAGE_PRESENT))
        return 0;
        
    return (pte & PAGE_MASK) | (virt & ~PAGE_MASK);
}

uintptr_t phys_to_virt(uintptr_t phys) {
    return phys;
}

size_t kmalloc_size(const void *ptr) {
    if (!ptr || !mm.initialized)
        return 0;
        
    for (int i = 0; i < 8; i++) {
        kmem_cache_t *cache = &mm.kmalloc_caches[i];
        slab_t *slab;
        
        list_for_each_entry(slab, &cache->slabs_full, list) {
            if ((uintptr_t)ptr >= (uintptr_t)(slab + 1) && 
                (uintptr_t)ptr < (uintptr_t)(slab + 1) + (PAGE_SIZE << cache->order)) {
                return cache->obj_size;
            }
        }
        
        list_for_each_entry(slab, &cache->slabs_partial, list) {
            if ((uintptr_t)ptr >= (uintptr_t)(slab + 1) && 
                (uintptr_t)ptr < (uintptr_t)(slab + 1) + (PAGE_SIZE << cache->order)) {
                return cache->obj_size;
            }
        }
    }
    
    page_t *page = (page_t *)((uintptr_t)ptr & PAGE_MASK);
    return PAGE_SIZE << page->order;
}

void mm_dump_stats(void) {
    if (!mm.initialized) {
        printf("Memory manager not initialized\n");
        return;
    }
    
    printf("Memory Manager Statistics:\n");
    printf("  Buddy Allocator:\n");
    printf("    Total free pages: %lu\n", mm.buddy.nr_free);
    for (int i = 0; i <= MAX_ORDER; i++) {
        int count = 0;
        list_head_t *pos;
        list_for_each(pos, &mm.buddy.free_area[i]) {
            count++;
        }
        printf("    Order %d: %d blocks\n", i, count);
    }
    
    printf("\n  Slab Allocators:\n");
    for (int i = 0; i < 8; i++) {
        kmem_cache_t *cache = &mm.kmalloc_caches[i];
        int full = 0, partial = 0, free = 0;
        slab_t *slab;
        
        list_for_each_entry(slab, &cache->slabs_full, list)
            full++;
            
        list_for_each_entry(slab, &cache->slabs_partial, list)
            partial++;
            
        list_for_each_entry(slab, &cache->slabs_free, list)
            free++;
        
        printf("    %s: objsize=%zu slabs(full=%d, partial=%d, free=%d)\n",
              cache->name, cache->obj_size, full, partial, free);
    }
}