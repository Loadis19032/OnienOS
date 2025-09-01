// framebuffer.h
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

// Структура для информации о фреймбуфере
struct framebuffer_info {
    uint32_t* address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint8_t type;
};

// Структуры Multiboot2
struct multiboot_header {
    uint32_t total_size;
    uint32_t reserved;
};

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t reserved;
};

// Константы Multiboot2
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_END 0

/**
 * @brief Получает информацию о фреймбуфере из структуры Multiboot2
 * 
 * @param magic Магическое число Multiboot2 (должно быть 0x36d76289)
 * @param mbi Указатель на структуру Multiboot2 information
 * @return Указатель на структуру с информацией о фреймбуфере или NULL если не найден
 */
static inline struct framebuffer_info* get_framebuffer_info(uint32_t magic, void* mbi) {
    // Проверяем магическое число
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        return NULL;
    }
    
    struct multiboot_header* header = (struct multiboot_header*)mbi;
    uint8_t* current_tag = (uint8_t*)mbi + sizeof(struct multiboot_header);
    
    // Ищем тег фреймбуфера
    while (1) {
        struct multiboot_tag* tag = (struct multiboot_tag*)current_tag;
        
        // Конец тегов
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }
        
        // Найден тег фреймбуфера
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)tag;
            
            // Статическая структура для возврата (можно изменить на динамическое выделение)
            static struct framebuffer_info fb_info;
            fb_info.address = (uint32_t*)(uintptr_t)fb_tag->framebuffer_addr;
            fb_info.width = fb_tag->framebuffer_width;
            fb_info.height = fb_tag->framebuffer_height;
            fb_info.pitch = fb_tag->framebuffer_pitch;
            fb_info.bpp = fb_tag->framebuffer_bpp;
            fb_info.type = fb_tag->framebuffer_type;
            
            return &fb_info;
        }
        
        // Переходим к следующему тегу (выравнивание на 8 байт)
        current_tag += (tag->size + 7) & ~7;
    }
    
    return NULL;
}

/**
 * @brief Получает адрес фреймбуфера (простая версия как в примере)
 * 
 * @param magic Магическое число Multiboot2
 * @param mbi Указатель на структуру Multiboot2 information
 * @return Указатель на фреймбуфер или NULL если не найден
 */
static inline uint32_t* get_framebuffer_address(uint32_t magic, void* mbi) {
    struct framebuffer_info* fb_info = get_framebuffer_info(magic, mbi);
    return fb_info ? fb_info->address : NULL;
}

#endif // FRAMEBUFFER_H