#ifndef PROS_H
#define PROS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../drivers/disk/pata/pata.h"
#include <stdbool.h>

#define PROS_SIGNATURE "PROSFS01"
#define PROS_MAX_NAME_LEN 64
#define PROS_SECTOR_SIZE 512
#define PROS_BOOT_SECTOR 1
#define PROS_FAT_ENTRY_FREE 0x00000000
#define PROS_FAT_ENTRY_EOF 0xFFFFFFFF
#define PROS_FAT_ENTRY_BAD 0xFFFFFFFE
#define PROS_MAX_FILES 128
#define PROS_DIR_ENTRY_EMPTY 0x00
#define PROS_DIR_ENTRY_DELETED 0xE5

typedef struct {
    char signature[8];
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_count;
    uint32_t cluster_count;
    uint32_t root_dir_cluster;
    uint32_t fat_size_sectors;
    uint32_t fat_start;
    uint32_t data_start;
    uint64_t total_sectors;
    uint64_t volume_id;
    char volume_label[8];
    uint8_t reserved[64];
} __attribute__((packed)) pros_boot_sector_t;

typedef struct {
    char name[64];
    uint8_t attributes;
    uint8_t reserved;
    uint64_t file_size;
    uint32_t start_cluster;
    uint32_t create_time;
    uint16_t last_access_date;
    uint16_t access_reserved;
    uint32_t modify_time;
    uint8_t reserved2[38];
} __attribute__((packed)) pros_dir_entry_t;

#define PROS_ATTR_READ_ONLY 0x01
#define PROS_ATTR_HIDDEN    0x02
#define PROS_ATTR_SYSTEM    0x04
#define PROS_ATTR_DIRECTORY 0x10
#define PROS_ATTR_ARCHIVE   0x20

typedef struct {
    char name[64];
    uint64_t size;
    uint32_t start_cluster;
    uint8_t attributes;
    bool is_open;
    uint32_t position;
} pros_file_t;

int pros_format(ata_device_t *dev);
int pros_init(ata_device_t *dev);
int pros_create_file(const char *name, uint8_t attributes);
int pros_rename_file(const char *old_name, const char *new_name);
int pros_write_file(const char *name, const void *data, size_t size, uint32_t offset);
int pros_read_file(const char *name, void *buffer, size_t size, uint32_t offset);
int pros_delete_file(const char *name);
int pros_list_files(void);
int pros_get_file_info(const char *name, pros_file_t *info);
int pros_open_file(const char *name, pros_file_t *file);
int pros_close_file(pros_file_t *file);
int pros_seek_file(pros_file_t *file, uint32_t offset);
int pros_read_open_file(pros_file_t *file, void *buffer, size_t size);
int pros_write_open_file(pros_file_t *file, const void *data, size_t size);
int pros_truncate_file(const char *name, uint64_t new_size);
int pros_create_directory(const char *name);
int pros_remove_directory(const char *name);
int pros_change_directory(const char *name);
int pros_get_free_space(uint64_t *free_bytes);
int pros_get_total_space(uint64_t *total_bytes);
int pros_defragment(void);

uint32_t pros_cluster_to_lba(uint32_t cluster);
uint32_t pros_find_free_cluster(void);
int pros_update_fat(uint32_t cluster, uint32_t value);
int pros_find_file(const char *name, pros_dir_entry_t *entry);
int pros_read_fat(uint32_t cluster, uint32_t *value);
int pros_get_cluster_chain(uint32_t start_cluster, uint32_t *chain, uint32_t max_clusters);
int pros_allocate_cluster_chain(uint32_t start_cluster, uint32_t clusters_needed);
int pros_free_cluster_chain(uint32_t start_cluster);
int pros_update_dir_entry(const char *name, const pros_dir_entry_t *entry);
int pros_find_free_dir_entry(uint32_t *cluster_idx, uint32_t *entry_idx);

extern ata_device_t *current_device;
extern pros_boot_sector_t boot_sector;
extern pros_file_t open_files[PROS_MAX_FILES];

#endif