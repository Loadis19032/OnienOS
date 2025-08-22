#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>
#include <stddef.h>

#include "../mm/mem.h"

// Multiboot2 magic number
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// Tag types
#define MULTIBOOT_TAG_TYPE_END                  0
#define MULTIBOOT_TAG_TYPE_CMDLINE             1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME    2
#define MULTIBOOT_TAG_TYPE_MODULE              3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO       4
#define MULTIBOOT_TAG_TYPE_BOOTDEV             5
#define MULTIBOOT_TAG_TYPE_MMAP                6
#define MULTIBOOT_TAG_TYPE_VBE                 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER         8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS        9
#define MULTIBOOT_TAG_TYPE_APM                 10
#define MULTIBOOT_TAG_TYPE_EFI32               11
#define MULTIBOOT_TAG_TYPE_EFI64               12
#define MULTIBOOT_TAG_TYPE_SMBIOS              13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD            14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW            15
#define MULTIBOOT_TAG_TYPE_NETWORK             16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP            17
#define MULTIBOOT_TAG_TYPE_EFI_BS              18
#define MULTIBOOT_TAG_TYPE_EFI32_IH            19
#define MULTIBOOT_TAG_TYPE_EFI64_IH            20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR      21

// Framebuffer types
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED     0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB         1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT    2

struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed));

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot_tag_end {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

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
    
    union {
        struct {
            uint32_t framebuffer_palette_addr;
            uint16_t framebuffer_palette_num_colors;
        } indexed;
        
        struct {
            uint8_t framebuffer_red_field_position;
            uint8_t framebuffer_red_mask_size;
            uint8_t framebuffer_green_field_position;
            uint8_t framebuffer_green_mask_size;
            uint8_t framebuffer_blue_field_position;
            uint8_t framebuffer_blue_mask_size;
        } rgb;
    };
} __attribute__((packed));

struct framebuffer_info {
    uint64_t address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    uint8_t type;
    
    uint8_t red_position;
    uint8_t red_mask_size;
    uint8_t green_position;
    uint8_t green_mask_size;
    uint8_t blue_position;
    uint8_t blue_mask_size;
    
    uint32_t palette_addr;
    uint16_t palette_colors;
};

static inline int is_valid_address(uint64_t addr, size_t size) {
    if (addr > UINT64_MAX - size) {
        return 0;
    }
    
    if (addr == 0) {
        return 0;
    }
    
    if (addr & 0x3) {
        return 0;
    }
    
    if (addr < 0x100000) {
        return 0;
    }
    
    return 1;
}

static inline void safe_memcpy(void *dest, const void *src, size_t n) {
    if (!dest || !src || n == 0) {
        return;
    }
    
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

struct framebuffer_info* parse_multiboot2_framebuffer(uint32_t magic, struct multiboot_info *mbi) {
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        return NULL;
    }
    
    if (!mbi || !is_valid_address((uint64_t)mbi, sizeof(struct multiboot_info))) {
        return NULL;
    }
    
    if (mbi->total_size < sizeof(struct multiboot_info)) {
        return NULL;
    }
    
    if (mbi->total_size > 1024 * 1024) {
        return NULL;
    }
    
    if (!is_valid_address((uint64_t)mbi, mbi->total_size)) {
        return NULL;
    }
    
    struct framebuffer_info *fb_info = NULL;
    
    struct multiboot_tag *tag = (struct multiboot_tag*)((uint8_t*)mbi + sizeof(struct multiboot_info));
    uint32_t parsed_size = sizeof(struct multiboot_info);
    
    while (parsed_size < mbi->total_size) {
        if (parsed_size + sizeof(struct multiboot_tag) > mbi->total_size) {
            break;
        }
        
        if (!is_valid_address((uint64_t)tag, sizeof(struct multiboot_tag))) {
            break;
        }
        
        if (tag->size < sizeof(struct multiboot_tag)) {
            break;
        }
        
        if (parsed_size + tag->size > mbi->total_size) {
            break;
        }
        
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            break;
        }
        
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            if (tag->size < sizeof(struct multiboot_tag_framebuffer)) {
                goto next_tag;
            }
            
            struct multiboot_tag_framebuffer *fb_tag = (struct multiboot_tag_framebuffer*)tag;
            
            if (fb_tag->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB && 
                fb_tag->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED) {
                goto next_tag;
            }
            
            if (fb_tag->framebuffer_width == 0 || fb_tag->framebuffer_height == 0 ||
                fb_tag->framebuffer_bpp == 0 || fb_tag->framebuffer_pitch == 0) {
                goto next_tag;
            }
            
            if (fb_tag->framebuffer_width > 32768 || fb_tag->framebuffer_height > 32768 ||
                fb_tag->framebuffer_bpp > 32 || fb_tag->framebuffer_pitch > 1024 * 1024) {
                goto next_tag;
            }
            
            uint64_t fb_size = (uint64_t)fb_tag->framebuffer_height * fb_tag->framebuffer_pitch;
            
            if (!is_valid_address(fb_tag->framebuffer_addr, fb_size)) {
                goto next_tag;
            }
            
            fb_info = (struct framebuffer_info*)kcalloc(1, sizeof(struct framebuffer_info));
            if (!fb_info) {
                return NULL;
            }
            
            fb_info->address = fb_tag->framebuffer_addr;
            fb_info->width = fb_tag->framebuffer_width;
            fb_info->height = fb_tag->framebuffer_height;
            fb_info->pitch = fb_tag->framebuffer_pitch;
            fb_info->bpp = fb_tag->framebuffer_bpp;
            fb_info->type = fb_tag->framebuffer_type;
            
            if (fb_tag->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
                fb_info->red_position = fb_tag->rgb.framebuffer_red_field_position;
                fb_info->red_mask_size = fb_tag->rgb.framebuffer_red_mask_size;
                fb_info->green_position = fb_tag->rgb.framebuffer_green_field_position;
                fb_info->green_mask_size = fb_tag->rgb.framebuffer_green_mask_size;
                fb_info->blue_position = fb_tag->rgb.framebuffer_blue_field_position;
                fb_info->blue_mask_size = fb_tag->rgb.framebuffer_blue_mask_size;
            } else if (fb_tag->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED) {
                fb_info->palette_addr = fb_tag->indexed.framebuffer_palette_addr;
                fb_info->palette_colors = fb_tag->indexed.framebuffer_palette_num_colors;
            }
            
            return fb_info;
        }
        
    next_tag:
        uint32_t aligned_size = (tag->size + 7) & ~7;
        parsed_size += aligned_size;
        tag = (struct multiboot_tag*)((uint8_t*)tag + aligned_size);
        
        if (parsed_size < aligned_size) {
            break;
        }
    }
    
    return NULL;
}

void free_framebuffer_info(struct framebuffer_info *fb_info) {
    if (fb_info) {
        kfree(fb_info);
    }
}

uint64_t get_framebuffer_address(uint32_t magic, struct multiboot_info *mbi) {
    struct framebuffer_info *fb_info = parse_multiboot2_framebuffer(magic, mbi);
    if (!fb_info) {
        return 0;
    }
    
    uint64_t address = fb_info->address;
    free_framebuffer_info(fb_info);
    return address;
}

#endif // MULTIBOOT2_H