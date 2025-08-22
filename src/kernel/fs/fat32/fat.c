#include "../include/fat.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MIN_CLUSTERS 65525
#define ENTRY_FREE 0xE5
#define ENTRY_LAST 0x00
#define LAST_LONG_ENTRY 0x40
#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_LENGTH 12
#define MAX_LONG_FILENAME 255
#define MAX_CLUSTER_CHAIN 65536

static int is_valid_cluster(fat32_fs_t* fs, uint32_t cluster) {
    if (!fs || !fs->device || cluster < 2 || cluster >= fs->TotalClusters + 2) {
        return 0;
    }
    return 1;
}

static uint32_t get_next_cluster(fat32_fs_t* fs, uint32_t cluster) {
    if (!fs || !fs->device || !is_valid_cluster(fs, cluster)) {
        return 0xFFFFFFFF;
    }

    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->FirstFATSector + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    uint8_t sector[512];
    if (ata_read_sectors(fs->device, fat_sector, 1, sector) != 0) {
        return 0xFFFFFFFF;
    }

    uint32_t next_cluster = *(uint32_t*)(sector + ent_offset);
    return next_cluster & 0x0FFFFFFF;
}

static int read_cluster_chain(fat32_fs_t* fs, uint32_t start_cluster, uint8_t* buffer, uint32_t max_size) {
    if (!fs || !fs->device || !buffer || max_size == 0 || !is_valid_cluster(fs, start_cluster)) {
        return -1;
    }

    uint32_t cluster = start_cluster;
    uint32_t offset = 0;
    uint32_t bytes_per_cluster = fs->bs.BPB_SecPerClus * 512;
    uint32_t clusters_read = 0;

    while (cluster < 0x0FFFFFF8 && offset < max_size && clusters_read < MAX_CLUSTER_CHAIN) {
        if (!is_valid_cluster(fs, cluster)) {
            return -1;
        }

        uint32_t sector = fs->FirstDataSector + (cluster - 2) * fs->bs.BPB_SecPerClus;
        if (sector >= fs->device->sectors) {
            return -1;
        }

        if (ata_read_sectors(fs->device, sector, fs->bs.BPB_SecPerClus, buffer + offset) != 0) {
            return -1;
        }

        offset += bytes_per_cluster;
        clusters_read++;
        cluster = get_next_cluster(fs, cluster);

        if (cluster == 0xFFFFFFFF) {
            return -1;
        }
    }

    return 0;
}

static uint32_t find_path_cluster(fat32_fs_t* fs, const char* path, uint32_t start_cluster) {
    if (!fs || !fs->device || !path || !is_valid_cluster(fs, start_cluster)) {
        return 0;
    }

    if (strcmp(path, "/") == 0 || strcmp(path, "\\") == 0 || strcmp(path, "") == 0) {
        return fs->bs.BPB_RootClus;
    }

    char* path_copy = strdup(path);
    if (!path_copy) {
        return 0;
    }

    char* token = strtok(path_copy, "/\\");
    uint32_t current_cluster = start_cluster;

    while (token != NULL) {
        if (!is_valid_cluster(fs, current_cluster)) {
            free(path_copy);
            return 0;
        }

        uint32_t bytes_per_cluster = fs->BytesPerCluster;
        uint8_t* cluster_data = malloc(bytes_per_cluster);
        if (!cluster_data) {
            free(path_copy);
            return 0;
        }

        if (read_cluster_chain(fs, current_cluster, cluster_data, bytes_per_cluster) != 0) {
            free(cluster_data);
            free(path_copy);
            return 0;
        }

        DirEntry* entries = (DirEntry*)cluster_data;
        int found = 0;
        char long_name[MAX_LONG_FILENAME + 1] = {0};

        for (int i = 0; i < bytes_per_cluster / sizeof(DirEntry); i++) {
            if (entries[i].name[0] == ENTRY_FREE) continue;
            if (entries[i].name[0] == ENTRY_LAST) break;
            if (entries[i].attr > 0x3F) continue;

            if (entries[i].attr == ATTR_LONG_NAME) {
                LfnEntry* lfn = (LfnEntry*)&entries[i];
                uint8_t seq_num = lfn->seq_num & 0x1F;

                if (seq_num > 0 && seq_num <= 20) {
                    for (int j = 0; j < 5; j++) {
                        if (lfn->name1[j] != 0xFFFF && lfn->name1[j] != 0) {
                            int pos = (seq_num - 1) * 13 + j;
                            if (pos < MAX_LONG_FILENAME) {
                                long_name[pos] = (char)(lfn->name1[j] & 0xFF);
                            }
                        }
                    }
                    for (int j = 0; j < 6; j++) {
                        if (lfn->name2[j] != 0xFFFF && lfn->name2[j] != 0) {
                            int pos = (seq_num - 1) * 13 + 5 + j;
                            if (pos < MAX_LONG_FILENAME) {
                                long_name[pos] = (char)(lfn->name2[j] & 0xFF);
                            }
                        }
                    }
                    for (int j = 0; j < 2; j++) {
                        if (lfn->name3[j] != 0xFFFF && lfn->name3[j] != 0) {
                            int pos = (seq_num - 1) * 13 + 11 + j;
                            if (pos < MAX_LONG_FILENAME) {
                                long_name[pos] = (char)(lfn->name3[j] & 0xFF);
                            }
                        }
                    }

                    if (lfn->seq_num & LAST_LONG_ENTRY) {
                        int len = 0;
                        while (len < MAX_LONG_FILENAME && long_name[len] != 0) len++;
                        long_name[len] = '\0';
                    }
                }
            } else {
                if (entries[i].attr & ATTR_VOLUME_ID) continue;

                int valid_name = 1;
                for (int j = 0; j < 8; j++) {
                    if (entries[i].name[j] != ' ' && (entries[i].name[j] < 0x20 || entries[i].name[j] > 0x7E)) {
                        valid_name = 0;
                        break;
                    }
                }
                if (!valid_name) continue;

                char short_name[MAX_FILENAME_LENGTH + 1] = {0};
                memcpy(short_name, entries[i].name, 8);
                memcpy(short_name + 8, entries[i].ext, 3);

                for (int j = 0; j < 11; j++) {
                    short_name[j] = tolower(short_name[j]);
                }

                int name_len = 8;
                while (name_len > 0 && short_name[name_len - 1] == ' ') name_len--;

                int ext_len = 3;
                while (ext_len > 0 && short_name[8 + ext_len - 1] == ' ') ext_len--;

                char converted_name[13] = {0};
                memcpy(converted_name, short_name, name_len);

                if (ext_len > 0) {
                    converted_name[name_len] = '.';
                    memcpy(converted_name + name_len + 1, short_name + 8, ext_len);
                }

                char search_token[MAX_LONG_FILENAME + 1];
                strncpy(search_token, token, MAX_LONG_FILENAME);
                search_token[MAX_LONG_FILENAME] = '\0';

                for (int j = 0; search_token[j]; j++) {
                    search_token[j] = tolower(search_token[j]);
                }

                if ((long_name[0] && strcmp(long_name, search_token) == 0) ||
                    (strcmp(converted_name, search_token) == 0)) {

                    uint32_t new_cluster = ((uint32_t)entries[i].fst_clus_hi << 16) | entries[i].fst_clus_lo;
                    if (is_valid_cluster(fs, new_cluster) || new_cluster == 0) {
                        current_cluster = new_cluster;
                        found = 1;
                        break;
                    }
                }

                long_name[0] = '\0';
            }
        }

        free(cluster_data);

        if (!found) {
            free(path_copy);
            return 0;
        }

        token = strtok(NULL, "/\\");
    }

    free(path_copy);
    return current_cluster;
}

int fat32_format(ata_device_t* dev) {
    if (!dev || !dev->exists) {
        return -1;
    }

    uint8_t sector[512] = {0};
    FAT32_BootSector* bs = (FAT32_BootSector*)sector;
    FAT32_FSInfo fsinfo = {0};
    uint32_t total_sectors = dev->sectors;

    if (total_sectors < 65536) {
        return -1;
    }

    uint8_t sec_per_clus = 8;
    uint32_t total_clusters = (total_sectors - 32) / sec_per_clus;
    uint32_t fat_size = (total_clusters * 4 + 511) / 512;

    bs->jmpBoot[0] = 0xEB;
    bs->jmpBoot[1] = 0x58;
    bs->jmpBoot[2] = 0x90;
    memcpy(bs->OEMName, "MYOS_FAT", 8);
    bs->BPB_BytsPerSec = 512;
    bs->BPB_SecPerClus = sec_per_clus;
    bs->BPB_RsvdSecCnt = 32;
    bs->BPB_NumFATs = 2;
    bs->BPB_RootEntCnt = 0;
    bs->BPB_TotSec16 = 0;
    bs->BPB_Media = 0xF8;
    bs->BPB_FATSz16 = 0;
    bs->BPB_SecPerTrk = 63;
    bs->BPB_NumHeads = 16;
    bs->BPB_HiddSec = 0;
    bs->BPB_TotSec32 = total_sectors;
    bs->BPB_FATSz32 = fat_size;
    bs->BPB_ExtFlags = 0;
    bs->BPB_FSVer = 0;
    bs->BPB_RootClus = 2;
    bs->BPB_FSInfo = 1;
    bs->BPB_BkBootSec = 6;
    memset(bs->BPB_Reserved, 0, sizeof(bs->BPB_Reserved));
    bs->BS_DrvNum = 0x80;
    bs->BS_Reserved1 = 0;
    bs->BS_BootSig = 0x29;
    bs->BS_VolID = 0x12345678;
    memcpy(bs->BS_VolLab, "MYOS_VOLUME", 11);
    memcpy(bs->BS_FilSysType, "FAT32   ", 8);
    bs->BootSignature = 0xAA55;

    if (ata_write_sectors(dev, 0, 1, bs) != 0) {
        return -1;
    }

    fsinfo.FSI_LeadSig = 0x41615252;
    fsinfo.FSI_StrucSig = 0x61417272;
    fsinfo.FSI_Free_Count = total_clusters - 1;
    fsinfo.FSI_Nxt_Free = 3;
    fsinfo.FSI_TrailSig = 0xAA550000;

    if (ata_write_sectors(dev, 1, 1, &fsinfo) != 0) {
        return -1;
    }

    uint32_t* fat = malloc(fat_size * 512);
    if (!fat) {
        return -1;
    }

    memset(fat, 0, fat_size * 512);
    fat[0] = 0x0FFFFFF8 | bs->BPB_Media;
    fat[1] = 0x0FFFFFFF;
    fat[2] = 0x0FFFFFFF;

    for (uint32_t i = 0; i < 2; i++) {
        uint32_t fat_offset = bs->BPB_RsvdSecCnt + i * fat_size;
        for (uint32_t j = 0; j < fat_size; j++) {
            if (ata_write_sectors(dev, fat_offset + j, 1, (uint8_t*)fat + j * 512) != 0) {
                free(fat);
                return -1;
            }
        }
    }

    free(fat);
    return 0;
}

int fat32_init(ata_device_t* dev, fat32_fs_t* fs) {
    if (!dev || !fs || !dev->exists) {
        return -1;
    }

    FAT32_BootSector bs;
    if (ata_read_sectors(dev, 0, 1, &bs) != 0) {
        return -1;
    }

    if (bs.BootSignature != 0xAA55) {
        return -1;
    }

    if (memcmp(bs.BS_FilSysType, "FAT32   ", 8) != 0) {
        return -1;
    }

    if (bs.BPB_FATSz32 == 0) {
        return -1;
    }

    fs->device = dev;
    fs->bs = bs;
    fs->FirstFATSector = bs.BPB_RsvdSecCnt;
    fs->FirstDataSector = bs.BPB_RsvdSecCnt + (bs.BPB_NumFATs * bs.BPB_FATSz32);
    fs->RootDirSector = fs->FirstDataSector + ((bs.BPB_RootClus - 2) * bs.BPB_SecPerClus);
    fs->TotalClusters = (bs.BPB_TotSec32 - fs->FirstDataSector) / bs.BPB_SecPerClus;
    fs->BytesPerCluster = bs.BPB_SecPerClus * 512;

    return 0;
}

int fat32_volume_exists(ata_device_t* dev) {
    if (!dev || !dev->exists) {
        return -1;
    }

    FAT32_BootSector bs;
    if (ata_read_sectors(dev, 0, 1, &bs) != 0) {
        return -1;
    }

    if (bs.BootSignature != 0xAA55) {
        return 0;
    }

    if (memcmp(bs.BS_FilSysType, "FAT32   ", 8) != 0) {
        return 0;
    }

    if (bs.BPB_FATSz16 != 0 || bs.BPB_RootEntCnt != 0) {
        return 0;
    }

    if (bs.BPB_FATSz32 == 0) {
        return 0;
    }

    FAT32_FSInfo fsinfo;
    if (ata_read_sectors(dev, bs.BPB_FSInfo, 1, &fsinfo) == 0) {
        if (fsinfo.FSI_LeadSig != 0x41615252 || fsinfo.FSI_StrucSig != 0x61417272) {
            return 0;
        }
    }

    return 1;
}

int fat32_exists(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->device || !path || !fs->device->exists) {
        return 0;
    }

    if (fs->bs.BootSignature != 0xAA55) {
        return 0;
    }

    if (memcmp(fs->bs.BS_FilSysType, "FAT32   ", 8) != 0) {
        return 0;
    }

    if (fs->bs.BPB_FATSz32 == 0) {
        return 0;
    }

    uint32_t cluster = find_path_cluster(fs, path, fs->bs.BPB_RootClus);
    return cluster != 0;
}

uint32_t fat32_get_cluster(fat32_fs_t* fs, const char* path) {
    if (!fs || !fs->device || !path || !fs->device->exists) {
        return 0;
    }

    return find_path_cluster(fs, path, fs->bs.BPB_RootClus);
}