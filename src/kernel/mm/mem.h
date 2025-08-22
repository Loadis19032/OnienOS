#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define PML4_SHIFT      39
#define PDP_SHIFT       30
#define PD_SHIFT        21
#define PT_SHIFT        12

#define PML4_INDEX(v)   (((v) >> PML4_SHIFT) & 0x1FF)
#define PDP_INDEX(v)    (((v) >> PDP_SHIFT) & 0x1FF)
#define PD_INDEX(v)     (((v) >> PD_SHIFT) & 0x1FF)
#define PT_INDEX(v)     (((v) >> PT_SHIFT) & 0x1FF)

#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1)
#define PAGE_USER       (1 << 2)
#define PAGE_PWT        (1 << 3)
#define PAGE_PCD        (1 << 4)
#define PAGE_ACCESSED   (1 << 5)
#define PAGE_DIRTY      (1 << 6)
#define PAGE_HUGE       (1 << 7)
#define PAGE_GLOBAL     (1 << 8)

#define MAX_ORDER       10
#define BUDDY_MAX_SIZE  (PAGE_SIZE << MAX_ORDER)

#define SLAB_MIN_SIZE   16
#define SLAB_MAX_SIZE   PAGE_SIZE

#define KMALLOC_MIN_SIZE SLAB_MIN_SIZE
#define KMALLOC_MAX_SIZE (PAGE_SIZE << 3)

#define ALIGN(x, a)     __ALIGN_MASK(x, (typeof(x))(a)-1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN_UP(x, a)  ALIGN((x), (a))
#define ALIGN_DOWN(x, a) ((x) & ~((typeof(x))(a)-1))

#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member) \
    for (pos = container_of((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = container_of(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = container_of((head)->next, typeof(*pos), member), \
         n = container_of(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = container_of(n->member.next, typeof(*n), member))

typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;

typedef struct page {
    list_head_t list;
    unsigned int order;
    unsigned int refcount;
    unsigned long flags;
} page_t;

typedef struct {
    list_head_t free_area[MAX_ORDER+1];
    unsigned long nr_free;
    uintptr_t base;
    size_t size;
} buddy_t;

// Предварительное объявление структуры kmem_cache
typedef struct kmem_cache kmem_cache_t;

typedef struct slab {
    list_head_t list;
    void *freelist;
    unsigned int inuse;
    unsigned int free;
    kmem_cache_t *cache;
} slab_t;

struct kmem_cache {
    char name[32];
    list_head_t slabs_full;
    list_head_t slabs_partial;
    list_head_t slabs_free;
    size_t obj_size;
    unsigned int objs_per_slab;
    unsigned int order;
    unsigned int objects;
    unsigned int pages;
};

typedef struct {
    buddy_t buddy;
    kmem_cache_t kmalloc_caches[8];
    uintptr_t phys_offset;
    bool initialized;
} mm_struct;

void mm_init(uintptr_t mem_start, uintptr_t mem_end);

void *kmalloc(size_t size);
void *kcalloc(size_t n, size_t size);
void *krealloc(void *p, size_t size);
void kfree(const void *ptr);

void *page_alloc(int order);
void page_free(void *addr, int order);

int map_pages(uintptr_t virt, uintptr_t phys, size_t count, uint64_t flags);
void unmap_pages(uintptr_t virt, size_t count);
uintptr_t virt_to_phys(uintptr_t virt);
uintptr_t phys_to_virt(uintptr_t phys);

kmem_cache_t *kmem_cache_create(const char *name, size_t size);
void *kmem_cache_alloc(kmem_cache_t *cache);
void kmem_cache_free(kmem_cache_t *cache, void *obj);
void kmem_cache_destroy(kmem_cache_t *cache);

size_t kmalloc_size(const void *ptr);
void mm_dump_stats(void);

extern uint64_t pml4_table_phys;
#define PML4_BASE (pml4_table_phys)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#define list_first_entry(ptr, type, member) \
    container_of((ptr)->next, type, member)

#define list_for_each_entry(pos, head, member) \
    for (pos = container_of((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = container_of(pos->member.next, typeof(*pos), member))

static inline void INIT_LIST_HEAD(list_head_t *list)
{
    list->next = list;
    list->prev = list;
}

static inline void list_add(list_head_t *new, list_head_t *head)
{
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

static inline void list_del(list_head_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = entry->prev = NULL;
}

static inline int list_empty(const list_head_t *head)
{
    return head->next == head;
}

#endif