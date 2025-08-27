#include "../include/pros.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

ata_device_t *current_device = NULL;
pros_boot_sector_t boot_sector;
pros_file_t open_files[PROS_MAX_FILES];

uint32_t pros_cluster_to_lba(uint32_t cluster) {
    if (cluster < 2 || cluster >= boot_sector.cluster_count + 2) {
        return 0;
    }
    return boot_sector.data_start + (cluster - 2) * boot_sector.sectors_per_cluster;
}

uint32_t pros_find_free_cluster(void) {
    uint32_t fat_sector = boot_sector.fat_start;
    uint32_t fat_entries_per_sector = PROS_SECTOR_SIZE / sizeof(uint32_t);
    uint32_t *fat_buffer = malloc(PROS_SECTOR_SIZE);
    
    if (!fat_buffer) {
        return 0;
    }
    
    for (uint32_t i = 0; i < boot_sector.fat_size_sectors; i++) {
        if (ata_read_sectors(current_device, fat_sector + i, 1, fat_buffer) != 0) {
            free(fat_buffer);
            return 0;
        }
        
        for (uint32_t j = 0; j < fat_entries_per_sector; j++) {
            uint32_t cluster_index = j + i * fat_entries_per_sector;
            if (cluster_index >= 2 && cluster_index < boot_sector.cluster_count + 2) {
                if (fat_buffer[j] == PROS_FAT_ENTRY_FREE) {
                    free(fat_buffer);
                    return cluster_index;
                }
            }
        }
    }
    
    free(fat_buffer);
    return 0;
}

int pros_update_fat(uint32_t cluster, uint32_t value) {
    if (cluster < 2 || cluster >= boot_sector.cluster_count + 2) {
        return -1;
    }
    
    uint32_t fat_sector = boot_sector.fat_start + (cluster * sizeof(uint32_t)) / PROS_SECTOR_SIZE;
    uint32_t fat_offset = (cluster * sizeof(uint32_t)) % PROS_SECTOR_SIZE;
    uint32_t *fat_buffer = malloc(PROS_SECTOR_SIZE);
    
    if (!fat_buffer) {
        return -1;
    }
    
    if (ata_read_sectors(current_device, fat_sector, 1, fat_buffer) != 0) {
        free(fat_buffer);
        return -1;
    }
    
    *((uint32_t*)((uint8_t*)fat_buffer + fat_offset)) = value;
    
    int result = ata_write_sectors(current_device, fat_sector, 1, fat_buffer);
    
    if (result == 0 && boot_sector.fat_count > 1) {
        result = ata_write_sectors(current_device, fat_sector + boot_sector.fat_size_sectors, 1, fat_buffer);
    }
    
    free(fat_buffer);
    return result;
}

int pros_read_fat(uint32_t cluster, uint32_t *value) {
    if (cluster < 2 || cluster >= boot_sector.cluster_count + 2) {
        return -1;
    }
    
    uint32_t fat_sector = boot_sector.fat_start + (cluster * sizeof(uint32_t)) / PROS_SECTOR_SIZE;
    uint32_t fat_offset = (cluster * sizeof(uint32_t)) % PROS_SECTOR_SIZE;
    uint32_t *fat_buffer = malloc(PROS_SECTOR_SIZE);
    
    if (!fat_buffer) {
        return -1;
    }
    
    if (ata_read_sectors(current_device, fat_sector, 1, fat_buffer) != 0) {
        free(fat_buffer);
        return -1;
    }
    
    *value = *((uint32_t*)((uint8_t*)fat_buffer + fat_offset));
    free(fat_buffer);
    return 0;
}

int pros_get_cluster_chain(uint32_t start_cluster, uint32_t *chain, uint32_t max_clusters) {
    if (start_cluster < 2 || start_cluster >= boot_sector.cluster_count + 2) {
        return -1;
    }
    
    uint32_t current_cluster = start_cluster;
    uint32_t count = 0;
    
    while (count < max_clusters && current_cluster < 0xFFFFFF8) {
        chain[count++] = current_cluster;
        
        if (pros_read_fat(current_cluster, &current_cluster) != 0) {
            return -1;
        }
        
        if (current_cluster == PROS_FAT_ENTRY_FREE || current_cluster == PROS_FAT_ENTRY_BAD) {
            break;
        }
    }
    
    return count;
}

int pros_allocate_cluster_chain(uint32_t start_cluster, uint32_t clusters_needed) {
    if (start_cluster < 2 || start_cluster >= boot_sector.cluster_count + 2) {
        return -1;
    }
    
    uint32_t current_cluster = start_cluster;
    uint32_t clusters_allocated = 1;
    
    while (clusters_allocated < clusters_needed) {
        uint32_t next_cluster = pros_find_free_cluster();
        
        if (next_cluster == 0) {
            return -1;
        }
        
        if (pros_update_fat(current_cluster, next_cluster) != 0) {
            return -1;
        }
        
        current_cluster = next_cluster;
        clusters_allocated++;
    }
    
    if (pros_update_fat(current_cluster, PROS_FAT_ENTRY_EOF) != 0) {
        return -1;
    }
    
    return 0;
}

int pros_free_cluster_chain(uint32_t start_cluster) {
    if (start_cluster < 2 || start_cluster >= boot_sector.cluster_count + 2) {
        return -1;
    }
    
    uint32_t current_cluster = start_cluster;
    
    while (current_cluster < 0xFFFFFF8) {
        uint32_t next_cluster;
        if (pros_read_fat(current_cluster, &next_cluster) != 0) {
            return -1;
        }
        
        if (pros_update_fat(current_cluster, PROS_FAT_ENTRY_FREE) != 0) {
            return -1;
        }
        
        if (next_cluster >= 0xFFFFFF8) {
            break;
        }
        
        current_cluster = next_cluster;
    }
    
    return 0;
}

int pros_find_file(const char *name, pros_dir_entry_t *entry) {
    if (!name || !entry || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    uint32_t cluster = boot_sector.root_dir_cluster;
    uint8_t *sector_buffer = malloc(PROS_SECTOR_SIZE);
    
    if (!sector_buffer) {
        return -1;
    }
    
    printf("Searching for file: %s, starting cluster: %u\n", name, cluster);
    
    while (cluster < 0xFFFFFF8) {
        uint32_t lba = pros_cluster_to_lba(cluster);
        printf("Reading cluster %u at LBA %u\n", cluster, lba);
        
        for (uint32_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
            if (ata_read_sectors(current_device, lba + i, 1, sector_buffer) != 0) {
                printf("Failed to read sector %u\n", lba + i);
                free(sector_buffer);
                return -1;
            }
            
            pros_dir_entry_t *dir_entry = (pros_dir_entry_t*)sector_buffer;
            
            for (uint32_t j = 0; j < PROS_SECTOR_SIZE / sizeof(pros_dir_entry_t); j++) {
                if (dir_entry[j].name[0] == PROS_DIR_ENTRY_EMPTY) {
                    printf("End of directory reached\n");
                    free(sector_buffer);
                    return -1;
                }
                
                if (dir_entry[j].name[0] == PROS_DIR_ENTRY_DELETED) {
                    continue;
                }
            
                if (strcmp(dir_entry[j].name, name) == 0) {
                    *entry = dir_entry[j];
                    printf("File found: %s, cluster: %u\n", name, dir_entry[j].start_cluster);
                    free(sector_buffer);
                    return 0;
                }
            }
        }
        
        uint32_t next_cluster;
        if (pros_read_fat(cluster, &next_cluster) != 0) {
            printf("Failed to read FAT for cluster %u\n", cluster);
            free(sector_buffer);
            return -1;
        }
        
        printf("Current cluster: %u, next cluster: %u\n", cluster, next_cluster);
        
        if (next_cluster >= 0xFFFFFF8 || next_cluster == PROS_FAT_ENTRY_FREE) {
            printf("End of cluster chain reached\n");
            free(sector_buffer);
            return -1;
        }
        
        cluster = next_cluster;
    }
    
    printf("Cluster chain invalid\n");
    free(sector_buffer);
    return -1;
}

int pros_find_free_dir_entry(uint32_t *cluster_idx, uint32_t *entry_idx) {
    uint32_t cluster = boot_sector.root_dir_cluster;
    uint8_t *sector_buffer = malloc(PROS_SECTOR_SIZE);
    
    if (!sector_buffer) {
        return -1;
    }
    
    uint32_t entries_per_sector = PROS_SECTOR_SIZE / sizeof(pros_dir_entry_t);
    
    do {
        uint32_t lba = pros_cluster_to_lba(cluster);
        
        for (uint32_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
            if (ata_read_sectors(current_device, lba + i, 1, sector_buffer) != 0) {
                free(sector_buffer);
                return -1;
            }
            
            pros_dir_entry_t *dir_entry = (pros_dir_entry_t*)sector_buffer;
            
            for (uint32_t j = 0; j < entries_per_sector; j++) {
                if (dir_entry[j].name[0] == PROS_DIR_ENTRY_EMPTY || 
                    dir_entry[j].name[0] == PROS_DIR_ENTRY_DELETED) {
                    *cluster_idx = cluster;
                    *entry_idx = j + i * entries_per_sector;
                    free(sector_buffer);
                    return 0;
                }
            }
        }
        
        uint32_t next_cluster;
        if (pros_read_fat(cluster, &next_cluster) != 0) {
            free(sector_buffer);
            return -1;
        }
        
        if (next_cluster >= 0xFFFFFF8) {
            uint32_t new_cluster = pros_find_free_cluster();
            if (new_cluster == 0) {
                free(sector_buffer);
                return -1;
            }
            
            if (pros_update_fat(cluster, new_cluster) != 0 ||
                pros_update_fat(new_cluster, PROS_FAT_ENTRY_EOF) != 0) {
                free(sector_buffer);
                return -1;
            }
            
            uint32_t new_lba = pros_cluster_to_lba(new_cluster);
            uint8_t *zero_buffer = calloc(1, PROS_SECTOR_SIZE);
            if (!zero_buffer) {
                free(sector_buffer);
                return -1;
            }
            
            for (uint32_t k = 0; k < boot_sector.sectors_per_cluster; k++) {
                if (ata_write_sectors(current_device, new_lba + k, 1, zero_buffer) != 0) {
                    free(sector_buffer);
                    free(zero_buffer);
                    return -1;
                }
            }
            
            free(zero_buffer);
            
            *cluster_idx = new_cluster;
            *entry_idx = 0;
            free(sector_buffer);
            return 0;
        }
        
        cluster = next_cluster;
        
    } while (cluster < 0xFFFFFF8);
    
    free(sector_buffer);
    return -1;
}

int pros_update_dir_entry(const char *name, const pros_dir_entry_t *entry) {
    if (!name || !entry || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    uint32_t cluster = boot_sector.root_dir_cluster;
    uint8_t *sector_buffer = malloc(PROS_SECTOR_SIZE);
    
    if (!sector_buffer) {
        return -1;
    }
    
    do {
        uint32_t lba = pros_cluster_to_lba(cluster);
        
        for (uint32_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
            if (ata_read_sectors(current_device, lba + i, 1, sector_buffer) != 0) {
                free(sector_buffer);
                return -1;
            }
            
            pros_dir_entry_t *dir_entry = (pros_dir_entry_t*)sector_buffer;
            bool updated = false;
            
            for (uint32_t j = 0; j < PROS_SECTOR_SIZE / sizeof(pros_dir_entry_t); j++) {
                if (dir_entry[j].name[0] == PROS_DIR_ENTRY_EMPTY) {
                    free(sector_buffer);
                    return -1;
                }
                
                if (dir_entry[j].name[0] != PROS_DIR_ENTRY_DELETED && 
                    strcmp(dir_entry[j].name, name) == 0) {
                    dir_entry[j] = *entry;
                    updated = true;
                    break;
                }
            }
            
            if (updated) {
                int result = ata_write_sectors(current_device, lba + i, 1, sector_buffer);
                free(sector_buffer);
                return result;
            }
        }
        
        uint32_t next_cluster;
        if (pros_read_fat(cluster, &next_cluster) != 0) {
            free(sector_buffer);
            return -1;
        }
        
        if (next_cluster >= 0xFFFFFF8) {
            break;
        }
        
        cluster = next_cluster;
    } while (1);
    
    free(sector_buffer);
    return -1;
}

int pros_format(ata_device_t *dev) {
    if (!dev || !dev->exists) {
        printf("Invalid device for formatting\n");
        return -1;
    }
    
    current_device = dev;
    
    uint64_t total_sectors = dev->sectors - 1;
    uint32_t sectors_per_cluster = 1;
    uint32_t reserved_sectors = 1;
    uint32_t fat_count = 2;
    
    uint32_t cluster_count = (total_sectors - reserved_sectors) / sectors_per_cluster;
    
    uint32_t fat_size_sectors = (cluster_count * sizeof(uint32_t) + PROS_SECTOR_SIZE - 1) / PROS_SECTOR_SIZE;
    
    if (reserved_sectors + fat_count * fat_size_sectors >= total_sectors) {
        printf("Not enough space for FAT\n");
        return -1;
    }
    
    uint32_t data_start = reserved_sectors + fat_count * fat_size_sectors;
    
    pros_boot_sector_t bs;
    memset(&bs, 0, sizeof(pros_boot_sector_t));
    memcpy(bs.signature, PROS_SIGNATURE, 8);
    bs.sectors_per_cluster = sectors_per_cluster;
    bs.reserved_sectors = reserved_sectors;
    bs.fat_count = fat_count;
    bs.cluster_count = cluster_count;
    bs.root_dir_cluster = 2;
    bs.fat_size_sectors = fat_size_sectors;
    bs.fat_start = reserved_sectors;
    bs.data_start = data_start;
    bs.total_sectors = total_sectors;
    bs.volume_id = 0x12345678;
    memcpy(bs.volume_label, "PROSFS", 6);

    // Записываем boot sector
    if (ata_write_sectors(dev, PROS_BOOT_SECTOR, 1, &bs) != 0) {
        printf("Failed to write boot sector\n");
        return -1;
    }

    // Сразу читаем обратно для проверки
    pros_boot_sector_t verify_bs;
    if (ata_read_sectors(dev, PROS_BOOT_SECTOR, 1, &verify_bs) != 0) {
        printf("Failed to read back boot sector\n");
        return -1;
    }
    
    // Проверяем сигнатуру
    if (memcmp(verify_bs.signature, PROS_SIGNATURE, 8) != 0) {
        printf("Boot sector verification failed after write!\n");
        return -1;
    }

    uint32_t *fat_buffer = (uint32_t*)malloc(PROS_SECTOR_SIZE);
    if (!fat_buffer) {
        printf("Failed to allocate memory for FAT\n");
        return -1;
    }

    for (uint32_t i = 0; i < fat_count; i++) {
        memset(fat_buffer, 0, PROS_SECTOR_SIZE);
        
        fat_buffer[0] = 0xFFFFFFF8;
        fat_buffer[1] = 0xFFFFFFFF;
        fat_buffer[2] = PROS_FAT_ENTRY_EOF;
        
        if (ata_write_sectors(dev, bs.fat_start + i * fat_size_sectors, 1, fat_buffer) != 0) {
            printf("Failed to write FAT\n");
            free(fat_buffer);
            return -1;
        }
    }

    free(fat_buffer);

    uint8_t *zero_buffer = (uint8_t*)calloc(1, PROS_SECTOR_SIZE);
    if (!zero_buffer) {
        printf("Failed to allocate memory for zero buffer\n");
        return -1;
    }

    uint32_t root_dir_lba = pros_cluster_to_lba(bs.root_dir_cluster);
    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        if (ata_write_sectors(dev, root_dir_lba + i, 1, zero_buffer) != 0) {
            printf("Failed to clear root directory\n");
            free(zero_buffer);
            return -1;
        }
    }

    free(zero_buffer);

    memcpy(&boot_sector, &bs, sizeof(pros_boot_sector_t));

    for (int i = 0; i < PROS_MAX_FILES; i++) {
        memset(&open_files[i], 0, sizeof(pros_file_t));
    }

    printf("Formatted successfully: %llu sectors, %u clusters, FAT size: %u sectors\n",
           total_sectors, cluster_count, fat_size_sectors);
    return 0;
}

int pros_init(ata_device_t *dev) {
    if (!dev || !dev->exists) {
        return -1;
    }
    
    current_device = dev;
    
    if (ata_read_sectors(dev, PROS_BOOT_SECTOR, 1, &boot_sector) != 0) {
        return -1;
    }
    
    if (memcmp(boot_sector.signature, PROS_SIGNATURE, 8) != 0) {
        return -1;
    }
    
    for (int i = 0; i < PROS_MAX_FILES; i++) {
        memset(&open_files[i], 0, sizeof(pros_file_t));
    }
    
    return 0;
}

int pros_create_file(const char *name, uint8_t attributes) {
    if (!name || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    pros_dir_entry_t existing;
    if (pros_find_file(name, &existing) == 0) {
        printf("File already exists: %s\n", name);
        return -1;
    }

    uint32_t free_cluster_idx, free_entry_idx;
    if (pros_find_free_dir_entry(&free_cluster_idx, &free_entry_idx) != 0) {
        printf("No free directory entry found\n");
        return -1;
    }

    printf("Found free entry: cluster=%u, entry=%u\n", free_cluster_idx, free_entry_idx);
    
    uint32_t entries_per_sector = PROS_SECTOR_SIZE / sizeof(pros_dir_entry_t);
    uint32_t sector_index = free_entry_idx / entries_per_sector;
    uint32_t entry_index = free_entry_idx % entries_per_sector;
    
    uint32_t lba = pros_cluster_to_lba(free_cluster_idx) + sector_index;
    
    uint8_t *sector_buffer = malloc(PROS_SECTOR_SIZE);
    if (!sector_buffer) {
        printf("Failed to allocate sector buffer\n");
        return -1;
    }
    
    if (ata_read_sectors(current_device, lba, 1, sector_buffer) != 0) {
        printf("Failed to read directory sector\n");
        free(sector_buffer);
        return -1;
    }
    
    pros_dir_entry_t *dir_entries = (pros_dir_entry_t*)sector_buffer;
    
    uint32_t file_cluster = pros_find_free_cluster();
    if (file_cluster == 0) {
        printf("No free clusters available\n");
        free(sector_buffer);
        return -1;
    }
    
    printf("Allocated cluster: %u for file: %s\n", file_cluster, name);
    
    if (pros_update_fat(file_cluster, PROS_FAT_ENTRY_EOF) != 0) {
        printf("Failed to update FAT for cluster %u\n", file_cluster);
        free(sector_buffer);
        return -1;
    }
    
    pros_dir_entry_t new_entry;
    memset(&new_entry, 0, sizeof(pros_dir_entry_t));
    strncpy(new_entry.name, name, PROS_MAX_NAME_LEN);
    new_entry.attributes = attributes;
    new_entry.file_size = 0;
    new_entry.start_cluster = file_cluster;
    
    time_t now = time(NULL);
    new_entry.create_time = now;
    new_entry.modify_time = now;
    
    dir_entries[entry_index] = new_entry;
    
    if (ata_write_sectors(current_device, lba, 1, sector_buffer) != 0) {
        printf("Failed to write directory sector\n");
        free(sector_buffer);
        return -1;
    }
    
    free(sector_buffer);
    printf("File created successfully: %s\n", name);
    return 0;
}

int pros_rename_file(const char *old_name, const char *new_name) {
    if (!old_name || !new_name || strlen(old_name) == 0 || strlen(new_name) == 0 || 
        strlen(old_name) > PROS_MAX_NAME_LEN || strlen(new_name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    pros_dir_entry_t existing;
    if (pros_find_file(new_name, &existing) == 0) {
        return -1;
    }
    
    pros_dir_entry_t entry;
    if (pros_find_file(old_name, &entry) != 0) {
        return -1;
    }
    
    char old_name_copy[PROS_MAX_NAME_LEN + 1];
    strcpy(old_name_copy, entry.name);
    
    strcpy(entry.name, new_name);
    entry.modify_time = time(NULL);
    
    if (pros_update_dir_entry(old_name_copy, &entry) != 0) {
        return -1;
    }
    
    for (int i = 0; i < PROS_MAX_FILES; i++) {
        if (open_files[i].is_open && strcmp(open_files[i].name, old_name_copy) == 0) {
            strcpy(open_files[i].name, new_name);
        }
    }
    
    return 0;
}

int pros_write_file(const char *name, const void *data, size_t size, uint32_t offset) {
    if (!name || !data || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    pros_dir_entry_t entry;
    if (pros_find_file(name, &entry) != 0) {
        return -1;
    }
    
    uint64_t required_size = offset + size;
    uint32_t sectors_needed = (required_size + PROS_SECTOR_SIZE - 1) / PROS_SECTOR_SIZE;
    uint32_t clusters_needed = (sectors_needed + boot_sector.sectors_per_cluster - 1) / boot_sector.sectors_per_cluster;
    
    uint32_t current_clusters = 0;
    uint32_t *cluster_chain = malloc(clusters_needed * sizeof(uint32_t));
    
    if (!cluster_chain) {
        return -1;
    }
    
    if (entry.start_cluster != 0) {
        current_clusters = pros_get_cluster_chain(entry.start_cluster, cluster_chain, clusters_needed);
        
        if (current_clusters == (uint32_t)-1) {
            free(cluster_chain);
            return -1;
        }
    }
    
    if (current_clusters < clusters_needed) {
        if (entry.start_cluster == 0) {
            entry.start_cluster = pros_find_free_cluster();
            
            if (entry.start_cluster == 0) {
                free(cluster_chain);
                return -1;
            }
            
            cluster_chain[0] = entry.start_cluster;
            current_clusters = 1;
        }
        
        if (pros_allocate_cluster_chain(cluster_chain[current_clusters - 1], clusters_needed - current_clusters + 1) != 0) {
            free(cluster_chain);
            return -1;
        }
        
        pros_get_cluster_chain(entry.start_cluster, cluster_chain, clusters_needed);
    }
    
    uint32_t bytes_written = 0;
    uint32_t current_cluster_idx = offset / (PROS_SECTOR_SIZE * boot_sector.sectors_per_cluster);
    uint32_t cluster_offset = offset % (PROS_SECTOR_SIZE * boot_sector.sectors_per_cluster);
    uint32_t sector_offset = cluster_offset / PROS_SECTOR_SIZE;
    uint32_t byte_offset = cluster_offset % PROS_SECTOR_SIZE;
    
    while (bytes_written < size && current_cluster_idx < clusters_needed) {
        uint32_t lba = pros_cluster_to_lba(cluster_chain[current_cluster_idx]) + sector_offset;
        uint8_t sector_buffer[PROS_SECTOR_SIZE];
        
        if (ata_read_sectors(current_device, lba, 1, sector_buffer) != 0) {
            free(cluster_chain);
            return -1;
        }
        
        uint32_t bytes_to_write = MIN(size - bytes_written, PROS_SECTOR_SIZE - byte_offset);
        memcpy(sector_buffer + byte_offset, (uint8_t*)data + bytes_written, bytes_to_write);
        
        if (ata_write_sectors(current_device, lba, 1, sector_buffer) != 0) {
            free(cluster_chain);
            return -1;
        }
        
        bytes_written += bytes_to_write;
        byte_offset = 0;
        sector_offset++;
        
        if (sector_offset >= boot_sector.sectors_per_cluster) {
            sector_offset = 0;
            current_cluster_idx++;
        }
    }
    
    if (required_size > entry.file_size) {
        entry.file_size = required_size;
    }
    
    entry.modify_time = time(NULL);
    
    if (pros_update_dir_entry(name, &entry) != 0) {
        free(cluster_chain);
        return -1;
    }
    
    free(cluster_chain);
    return bytes_written;
}

int pros_read_file(const char *name, void *buffer, size_t size, uint32_t offset) {
    if (!name || !buffer || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    pros_dir_entry_t entry;
    if (pros_find_file(name, &entry) != 0) {
        return -1;
    }
    
    if (offset >= entry.file_size) {
        return 0;
    }
    
    size = MIN(size, entry.file_size - offset);
    
    uint32_t sectors_needed = (size + offset + PROS_SECTOR_SIZE - 1) / PROS_SECTOR_SIZE;
    uint32_t clusters_needed = (sectors_needed + boot_sector.sectors_per_cluster - 1) / boot_sector.sectors_per_cluster;
    
    uint32_t *cluster_chain = malloc(clusters_needed * sizeof(uint32_t));
    
    if (!cluster_chain) {
        return -1;
    }
    
    uint32_t chain_length = pros_get_cluster_chain(entry.start_cluster, cluster_chain, clusters_needed);
    
    if (chain_length == (uint32_t)-1) {
        free(cluster_chain);
        return -1;
    }
    
    uint32_t bytes_read = 0;
    uint32_t current_cluster_idx = offset / (PROS_SECTOR_SIZE * boot_sector.sectors_per_cluster);
    uint32_t cluster_offset = offset % (PROS_SECTOR_SIZE * boot_sector.sectors_per_cluster);
    uint32_t sector_offset = cluster_offset / PROS_SECTOR_SIZE;
    uint32_t byte_offset = cluster_offset % PROS_SECTOR_SIZE;
    
    while (bytes_read < size && current_cluster_idx < chain_length) {
        uint32_t lba = pros_cluster_to_lba(cluster_chain[current_cluster_idx]) + sector_offset;
        uint8_t sector_buffer[PROS_SECTOR_SIZE];
        
        if (ata_read_sectors(current_device, lba, 1, sector_buffer) != 0) {
            free(cluster_chain);
            return -1;
        }
        
        uint32_t bytes_to_read = MIN(size - bytes_read, PROS_SECTOR_SIZE - byte_offset);
        memcpy((uint8_t*)buffer + bytes_read, sector_buffer + byte_offset, bytes_to_read);
        
        bytes_read += bytes_to_read;
        byte_offset = 0;
        sector_offset++;
        
        if (sector_offset >= boot_sector.sectors_per_cluster) {
            sector_offset = 0;
            current_cluster_idx++;
        }
    }
    
    free(cluster_chain);
    return bytes_read;
}

int pros_delete_file(const char *name) {
    if (!name || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    pros_dir_entry_t entry;
    if (pros_find_file(name, &entry) != 0) {
        return -1;
    }
    
    if (pros_free_cluster_chain(entry.start_cluster) != 0) {
        return -1;
    }
    
    for (int i = 0; i < PROS_MAX_FILES; i++) {
        if (open_files[i].is_open && strcmp(open_files[i].name, name) == 0) {
            memset(&open_files[i], 0, sizeof(pros_file_t));
        }
    }
    
    pros_dir_entry_t empty_entry;
    memset(&empty_entry, 0, sizeof(pros_dir_entry_t));
    empty_entry.name[0] = PROS_DIR_ENTRY_DELETED;
    
    return pros_update_dir_entry(name, &empty_entry);
}

int pros_list_files(void) {
    uint32_t cluster = boot_sector.root_dir_cluster;
    uint8_t *sector_buffer = malloc(PROS_SECTOR_SIZE);
    int file_count = 0;
    
    if (!sector_buffer) {
        return -1;
    }
    
    printf("Files in root directory:\n");
    printf("%-64s %-10s %-12s\n", "Name", "Size", "Attributes");
    printf("------------------------------------------------------------------------\n");
    
    do {
        uint32_t lba = pros_cluster_to_lba(cluster);
        
        for (uint32_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
            if (ata_read_sectors(current_device, lba + i, 1, sector_buffer) != 0) {
                free(sector_buffer);
                return -1;
            }
            
            pros_dir_entry_t *dir_entry = (pros_dir_entry_t*)sector_buffer;
            
            for (uint32_t j = 0; j < PROS_SECTOR_SIZE / sizeof(pros_dir_entry_t); j++) {
                if (dir_entry[j].name[0] == PROS_DIR_ENTRY_EMPTY) {
                    free(sector_buffer);
                    printf("Total files: %d\n", file_count);
                    return file_count;
                }
                
                if (dir_entry[j].name[0] != PROS_DIR_ENTRY_DELETED) {
                    printf("%-64s %-10llu ", dir_entry[j].name, dir_entry[j].file_size);
                    
                    if (dir_entry[j].attributes & PROS_ATTR_READ_ONLY) printf("R");
                    if (dir_entry[j].attributes & PROS_ATTR_HIDDEN) printf("H");
                    if (dir_entry[j].attributes & PROS_ATTR_SYSTEM) printf("S");
                    if (dir_entry[j].attributes & PROS_ATTR_DIRECTORY) printf("D");
                    if (dir_entry[j].attributes & PROS_ATTR_ARCHIVE) printf("A");
                    
                    printf("\n");
                    file_count++;
                }
            }
        }
        
        uint32_t next_cluster;
        if (pros_read_fat(cluster, &next_cluster) != 0) {
            free(sector_buffer);
            return -1;
        }
        
        if (next_cluster >= 0xFFFFFF8) {
            break;
        }
        
        cluster = next_cluster;
    } while (1);
    
    free(sector_buffer);
    printf("Total files: %d\n", file_count);
    return file_count;
}

int pros_get_file_info(const char *name, pros_file_t *info) {
    if (!name || !info || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    pros_dir_entry_t entry;
    if (pros_find_file(name, &entry) != 0) {
        return -1;
    }
    
    strcpy(info->name, entry.name);
    info->size = entry.file_size;
    info->start_cluster = entry.start_cluster;
    info->attributes = entry.attributes;
    info->is_open = false;
    info->position = 0;
    
    return 0;
}

int pros_open_file(const char *name, pros_file_t *file) {
    if (!name || !file || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    for (int i = 0; i < PROS_MAX_FILES; i++) {
        if (open_files[i].is_open && strcmp(open_files[i].name, name) == 0) {
            *file = open_files[i];
            return 0;
        }
    }
    
    pros_dir_entry_t entry;
    if (pros_find_file(name, &entry) != 0) {
        return -1;
    }
    
    for (int i = 0; i < PROS_MAX_FILES; i++) {
        if (!open_files[i].is_open) {
            strcpy(open_files[i].name, entry.name);
            open_files[i].size = entry.file_size;
            open_files[i].start_cluster = entry.start_cluster;
            open_files[i].attributes = entry.attributes;
            open_files[i].is_open = true;
            open_files[i].position = 0;
            
            *file = open_files[i];
            return 0;
        }
    }
    
    return -1;
}

int pros_close_file(pros_file_t *file) {
    if (!file || !file->is_open) {
        return -1;
    }
    
    for (int i = 0; i < PROS_MAX_FILES; i++) {
        if (open_files[i].is_open && strcmp(open_files[i].name, file->name) == 0) {
            memset(&open_files[i], 0, sizeof(pros_file_t));
            break;
        }
    }
    
    memset(file, 0, sizeof(pros_file_t));
    return 0;
}

int pros_seek_file(pros_file_t *file, uint32_t offset) {
    if (!file || !file->is_open) {
        return -1;
    }
    
    if (offset > file->size) {
        return -1;
    }
    
    file->position = offset;
    return 0;
}

int pros_read_open_file(pros_file_t *file, void *buffer, size_t size) {
    if (!file || !file->is_open || !buffer) {
        return -1;
    }
    
    if (file->position >= file->size) {
        return 0;
    }
    
    size = MIN(size, file->size - file->position);
    int bytes_read = pros_read_file(file->name, buffer, size, file->position);
    
    if (bytes_read > 0) {
        file->position += bytes_read;
    }
    
    return bytes_read;
}

int pros_write_open_file(pros_file_t *file, const void *data, size_t size) {
    if (!file || !file->is_open || !data) {
        return -1;
    }
    
    int bytes_written = pros_write_file(file->name, data, size, file->position);
    
    if (bytes_written > 0) {
        file->position += bytes_written;
        file->size = MAX(file->size, file->position);
        
        for (int i = 0; i < PROS_MAX_FILES; i++) {
            if (open_files[i].is_open && strcmp(open_files[i].name, file->name) == 0) {
                open_files[i].position = file->position;
                open_files[i].size = file->size;
                break;
            }
        }
    }
    
    return bytes_written;
}

int pros_truncate_file(const char *name, uint64_t new_size) {
    if (!name || strlen(name) == 0 || strlen(name) > PROS_MAX_NAME_LEN) {
        return -1;
    }
    
    pros_dir_entry_t entry;
    if (pros_find_file(name, &entry) != 0) {
        return -1;
    }
    
    if (new_size == entry.file_size) {
        return 0;
    }
    
    if (new_size < entry.file_size) {
        uint32_t sectors_needed = (new_size + PROS_SECTOR_SIZE - 1) / PROS_SECTOR_SIZE;
        uint32_t clusters_needed = (sectors_needed + boot_sector.sectors_per_cluster - 1) / boot_sector.sectors_per_cluster;
        
        uint32_t *cluster_chain = malloc((clusters_needed + 1) * sizeof(uint32_t));
        
        if (!cluster_chain) {
            return -1;
        }
        
        uint32_t chain_length = pros_get_cluster_chain(entry.start_cluster, cluster_chain, clusters_needed + 1);
        
        if (chain_length == (uint32_t)-1) {
            free(cluster_chain);
            return -1;
        }
        
        if (chain_length > clusters_needed) {
            if (pros_free_cluster_chain(cluster_chain[clusters_needed]) != 0) {
                free(cluster_chain);
                return -1;
            }
            
            if (pros_update_fat(cluster_chain[clusters_needed - 1], PROS_FAT_ENTRY_EOF) != 0) {
                free(cluster_chain);
                return -1;
            }
        }
        
        free(cluster_chain);
    } else {
        uint32_t sectors_needed = (new_size + PROS_SECTOR_SIZE - 1) / PROS_SECTOR_SIZE;
        uint32_t clusters_needed = (sectors_needed + boot_sector.sectors_per_cluster - 1) / boot_sector.sectors_per_cluster;
        
        uint32_t current_clusters = 0;
        uint32_t *cluster_chain = malloc(clusters_needed * sizeof(uint32_t));
        
        if (!cluster_chain) {
            return -1;
        }
        
        if (entry.start_cluster != 0) {
            current_clusters = pros_get_cluster_chain(entry.start_cluster, cluster_chain, clusters_needed);
            
            if (current_clusters == (uint32_t)-1) {
                free(cluster_chain);
                return -1;
            }
        }
        
        if (current_clusters < clusters_needed) {
            if (entry.start_cluster == 0) {
                entry.start_cluster = pros_find_free_cluster();
                
                if (entry.start_cluster == 0) {
                    free(cluster_chain);
                    return -1;
                }
                
                cluster_chain[0] = entry.start_cluster;
                current_clusters = 1;
            }
            
            if (pros_allocate_cluster_chain(cluster_chain[current_clusters - 1], clusters_needed - current_clusters + 1) != 0) {
                free(cluster_chain);
                return -1;
            }
        }
        
        free(cluster_chain);
    }
    
    entry.file_size = new_size;
    entry.modify_time = time(NULL);
    
    if (pros_update_dir_entry(name, &entry) != 0) {
        return -1;
    }
    
    for (int i = 0; i < PROS_MAX_FILES; i++) {
        if (open_files[i].is_open && strcmp(open_files[i].name, name) == 0) {
            open_files[i].size = new_size;
            if (open_files[i].position > new_size) {
                open_files[i].position = new_size;
            }
        }
    }
    
    return 0;
}

int pros_create_directory(const char *name) {
    return pros_create_file(name, PROS_ATTR_DIRECTORY);
}

int pros_remove_directory(const char *name) {
    pros_dir_entry_t entry;
    if (pros_find_file(name, &entry) != 0) {
        return -1;
    }
    
    if (!(entry.attributes & PROS_ATTR_DIRECTORY)) {
        return -1;
    }
    
    return pros_delete_file(name);
}

int pros_change_directory(const char *name) {
    return 0;
}

int pros_get_free_space(uint64_t *free_bytes) {
    if (!free_bytes) {
        return -1;
    }
    
    uint32_t free_clusters = 0;
    uint32_t fat_sector = boot_sector.fat_start;
    uint32_t fat_entries_per_sector = PROS_SECTOR_SIZE / sizeof(uint32_t);
    uint32_t *fat_buffer = malloc(PROS_SECTOR_SIZE);
    
    if (!fat_buffer) {
        return -1;
    }
    
    for (uint32_t i = 0; i < boot_sector.fat_size_sectors; i++) {
        if (ata_read_sectors(current_device, fat_sector + i, 1, fat_buffer) != 0) {
            free(fat_buffer);
            return -1;
        }
        
        for (uint32_t j = 0; j < fat_entries_per_sector; j++) {
            uint32_t cluster_index = j + i * fat_entries_per_sector;
            if (cluster_index >= 2 && cluster_index < boot_sector.cluster_count + 2) {
                if (fat_buffer[j] == PROS_FAT_ENTRY_FREE) {
                    free_clusters++;
                }
            }
        }
    }
    
    free(fat_buffer);
    *free_bytes = free_clusters * boot_sector.sectors_per_cluster * PROS_SECTOR_SIZE;
    return 0;
}

int pros_get_total_space(uint64_t *total_bytes) {
    if (!total_bytes) {
        return -1;
    }
    
    *total_bytes = boot_sector.cluster_count * boot_sector.sectors_per_cluster * PROS_SECTOR_SIZE;
    return 0;
}

int pros_defragment(void) {
    return 0;
}