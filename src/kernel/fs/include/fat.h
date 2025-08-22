#ifndef _FAT32_H
#define _FAT32_H

#include <stdint.h>
#include <stddef.h>
#include "../../../drivers/disk/pata/pata.h"
#include <stdbool.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t jmpBoot[3];
    char OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
    uint8_t BootCode[420];
    uint16_t BootSignature;
} FAT32_BootSector;

typedef struct {
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint16_t FSI_TrailSig;
} FAT32_FSInfo;

// Standard 8.3 directory entry
typedef struct {
    char name[8];
    char ext[3];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} DirEntry;

// Long filename directory entry
typedef struct {
    uint8_t seq_num;
    uint16_t name1[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t fst_clus_lo;
    uint16_t name3[2];
} LfnEntry;
#pragma pack(pop)

typedef struct {
    ata_device_t* device;
    FAT32_BootSector bs;
    uint32_t FirstDataSector;
    uint32_t FirstFATSector;
    uint32_t RootDirSector;
    uint32_t TotalClusters;
    uint32_t BytesPerCluster;
} fat32_fs_t;

// File/directory attributes
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

// File modes
#define FAT32_MODE_READ 0x01
#define FAT32_MODE_WRITE 0x02
#define FAT32_MODE_CREATE 0x04

// File handle
typedef struct {
    fat32_fs_t* fs;
    uint32_t start_cluster;
    uint32_t current_cluster;
    uint32_t file_size;
    uint32_t position;
    uint8_t mode;
    uint32_t dir_sector;
    uint32_t dir_offset;
} fat32_file_t;

int fat32_create(fat32_fs_t* fs, const char* path);
int fat32_open(fat32_fs_t* fs, const char* path, uint8_t mode, fat32_file_t* file);
int fat32_close(fat32_file_t* file);

int fat32_format(ata_device_t* dev);
int fat32_init(ata_device_t* dev, fat32_fs_t* fs);
int fat32_volume_exists(ata_device_t* dev);
int fat32_exists(fat32_fs_t* fs, const char* path);
uint32_t fat32_get_cluster(fat32_fs_t* fs, const char* path);

#endif