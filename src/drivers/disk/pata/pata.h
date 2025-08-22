#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_PRIMARY_BASE    0x1F0
#define ATA_SECONDARY_BASE  0x170

#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECCOUNT    0x02
#define ATA_REG_LBA_LOW     0x03
#define ATA_REG_LBA_MID     0x04
#define ATA_REG_LBA_HIGH    0x05
#define ATA_REG_DRIVE       0x06
#define ATA_REG_STATUS      0x07
#define ATA_REG_COMMAND     0x07

#define ATA_REG_CONTROL     0x206
#define ATA_REG_ALTSTATUS   0x206

#define ATA_STATUS_BSY      0x80
#define ATA_STATUS_DRDY     0x40
#define ATA_STATUS_DF       0x20
#define ATA_STATUS_DSC      0x10
#define ATA_STATUS_DRQ      0x08
#define ATA_STATUS_CORR     0x04
#define ATA_STATUS_IDX      0x02
#define ATA_STATUS_ERR      0x01

#define ATA_ERROR_BBK       0x80
#define ATA_ERROR_UNC       0x40
#define ATA_ERROR_MC        0x20
#define ATA_ERROR_IDNF      0x10
#define ATA_ERROR_MCR       0x08
#define ATA_ERROR_ABRT      0x04
#define ATA_ERROR_TK0NF     0x02
#define ATA_ERROR_AMNF      0x01

#define ATA_CMD_READ_SECTORS    0x20
#define ATA_CMD_WRITE_SECTORS   0x30
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_FLUSH_CACHE     0xE7

#define ATA_DRIVE_MASTER    0xA0
#define ATA_DRIVE_SLAVE     0xB0

#define ATA_SECTOR_SIZE     512

typedef struct {
    uint16_t base;       
    uint16_t control;    
    uint8_t drive;       
    uint8_t exists;      
    char model[41];      
    uint32_t sectors;    
} ata_device_t;

int ata_init(void);
int ata_identify(ata_device_t *dev);
int ata_read_sectors(ata_device_t *dev, uint32_t lba, uint8_t count, void *buffer);
int ata_write_sectors(ata_device_t *dev, uint32_t lba, uint8_t count, const void *buffer);
void ata_wait_busy(ata_device_t *dev);
int ata_wait_ready(ata_device_t *dev);
uint8_t ata_get_status(ata_device_t *dev);
void ata_select_drive(ata_device_t *dev, uint32_t lba);
void ata_reset_controller(uint16_t base);

extern ata_device_t ata_devices[4];
extern ata_device_t* get_ata_device(int drive);

#endif // ATA_H