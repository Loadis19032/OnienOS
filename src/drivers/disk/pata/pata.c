#include <asm/io.h>
#include <stdio.h>
#include <string.h>
#include "pata.h"

ata_device_t ata_devices[4];

static void ata_delay(void) {
    for (int i = 0; i < 4; i++) {
        inb(ATA_PRIMARY_BASE + ATA_REG_ALTSTATUS);
    }
}

static void ata_long_delay(void) {
    for (int i = 0; i < 1000; i++) {
        ata_delay();
    }
}

uint8_t ata_get_status(ata_device_t *dev) {
    return inb(dev->base + ATA_REG_STATUS);
}

void ata_wait_busy(ata_device_t *dev) {
    while (ata_get_status(dev) & ATA_STATUS_BSY);
}

int ata_wait_ready(ata_device_t *dev) {
    ata_wait_busy(dev);
    
    uint8_t status = ata_get_status(dev);
    
    if (status & ATA_STATUS_ERR) {
        uint8_t error = inb(dev->base + ATA_REG_ERROR);
        
        if (error == ATA_ERROR_ABRT || error == ATA_ERROR_IDNF) {
            return 0; 
        }
        
        printf("ATA Error: 0x%X", error);
        
        if (error & ATA_ERROR_UNC) printf(" (Uncorrectable Data)");
        if (error & ATA_ERROR_BBK) printf(" (Bad Block)");
        if (error & ATA_ERROR_MC) printf(" (Media Changed)");
        printf("\n");
        
        return -1;
    }
    
    if (status & ATA_STATUS_DF) {
        printf("ATA Drive Fault\n");
        return -1;
    }
    
    return 0;
}

void ata_select_drive(ata_device_t *dev, uint32_t lba) {
    uint8_t drive_reg = (dev->drive == 0 ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE) | 
                        ((lba >> 24) & 0x0F);
    
    outb(dev->base + ATA_REG_DRIVE, drive_reg);
    ata_long_delay();
}

int ata_identify(ata_device_t *dev) {
    outb(dev->base + ATA_REG_DRIVE, dev->drive == 0 ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE);
    ata_long_delay();

    uint8_t status = ata_get_status(dev);
    if (status == 0 || status == 0xFF) {
        return -1;
    }
    
    outb(dev->base + ATA_REG_SECCOUNT, 0);
    outb(dev->base + ATA_REG_LBA_LOW, 0);
    outb(dev->base + ATA_REG_LBA_MID, 0);
    outb(dev->base + ATA_REG_LBA_HIGH, 0);
    
    outb(dev->base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay();
    
    status = ata_get_status(dev);
    if (status == 0) {
        return -1;
    }
    
    ata_wait_ready(dev);
    
    uint8_t lba_mid = inb(dev->base + ATA_REG_LBA_MID);
    uint8_t lba_high = inb(dev->base + ATA_REG_LBA_HIGH);
    if (lba_mid != 0 || lba_high != 0) {
        return -1;
    }
    
    int timeout = 1000000;
    while (!(ata_get_status(dev) & ATA_STATUS_DRQ) && timeout > 0) {
        if (ata_get_status(dev) & ATA_STATUS_ERR) {
            break;
        }
        timeout--;
    }
    
    if (timeout == 0) {
        printf("ATA Identify timeout\n");
        return -1;
    }
    
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(dev->base + ATA_REG_DATA);
    }
    
    memset(dev->model, 0, sizeof(dev->model));
    for (int i = 0; i < 20; i++) {
        uint16_t word = identify_data[27 + i];
        dev->model[i * 2] = (word >> 8) & 0xFF;
        dev->model[i * 2 + 1] = word & 0xFF;
    }
    dev->model[40] = '\0';
    
    for (int i = 39; i >= 0 && dev->model[i] == ' '; i--) {
        dev->model[i] = '\0';
    }
    
    dev->sectors = (uint32_t)identify_data[60] | ((uint32_t)identify_data[61] << 16);
    
    dev->exists = 1;
    printf("ATA Device %d: %s (%u sectors)\n", dev->drive, dev->model, dev->sectors);
    
    return 0;
}

int ata_read_sectors(ata_device_t *dev, uint32_t lba, uint8_t count, void *buffer) {
    if (!dev->exists || count == 0) {
        printf("dev exists: %d, count: %d", dev->exists, count);
        return -1;
    }
    
    ata_select_drive(dev, lba);
    ata_wait_busy(dev);

    outb(dev->base + ATA_REG_FEATURES, 0);
    outb(dev->base + ATA_REG_SECCOUNT, count);
    outb(dev->base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(dev->base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(dev->base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    
    outb(dev->base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
    
    uint16_t *buf = (uint16_t *)buffer;
    
    for (int sector = 0; sector < count; sector++) {
        ata_wait_ready(dev);
        
        int timeout = 1000000;
        while (!(ata_get_status(dev) & ATA_STATUS_DRQ) && timeout > 0) {
            uint8_t status = ata_get_status(dev);
            if (status & ATA_STATUS_ERR) {
                uint8_t error = inb(dev->base + ATA_REG_ERROR);
                if (error != ATA_ERROR_ABRT && error != ATA_ERROR_IDNF) {
                    printf("ATA Read Error: 0x%X\n", error);
                    return -1;
                }
                break; 
            }
            timeout--;
        }
        
        if (timeout == 0) {
            printf("ATA Read timeout\n");
            return -1;
        }
        
        for (int i = 0; i < 256; i++) {
            buf[sector * 256 + i] = inw(dev->base + ATA_REG_DATA);
        }
        
        ata_delay(); 
    }
    
    return 0;
}

int ata_write_sectors(ata_device_t *dev, uint32_t lba, uint8_t count, const void *buffer) {
    if (!dev->exists || count == 0) {
        return -1;
    }
    
    ata_select_drive(dev, lba);
    ata_wait_busy(dev);
    
    outb(dev->base + ATA_REG_FEATURES, 0);
    outb(dev->base + ATA_REG_SECCOUNT, count);
    outb(dev->base + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(dev->base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(dev->base + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    
    outb(dev->base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    const uint16_t *buf = (const uint16_t *)buffer;
    
    for (int sector = 0; sector < count; sector++) {
        ata_wait_ready(dev);
        
        int timeout = 1000000;
        while (!(ata_get_status(dev) & ATA_STATUS_DRQ) && timeout > 0) {
            uint8_t status = ata_get_status(dev);
            if (status & ATA_STATUS_ERR) {
                uint8_t error = inb(dev->base + ATA_REG_ERROR);
                if (error != ATA_ERROR_ABRT && error != ATA_ERROR_IDNF) {
                    printf("ATA Write Error: 0x%02X\n", error);
                    return -1;
                }
                break; 
            }
            timeout--;
        }
        
        if (timeout == 0) {
            printf("ATA Write timeout\n");
            return -1;
        }
        
        for (int i = 0; i < 256; i++) {
            outw(dev->base + ATA_REG_DATA, buf[sector * 256 + i]);
        }
        
        ata_delay();
    }
    
    outb(dev->base + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);
    ata_wait_ready(dev);
    
    return 0;
}

void ata_reset_controller(uint16_t base) {
    outb(base + ATA_REG_CONTROL, 0x04);
    ata_long_delay();
    
    outb(base + ATA_REG_CONTROL, 0x00);
    ata_long_delay();
    
    int timeout = 1000000;
    while ((inb(base + ATA_REG_STATUS) & ATA_STATUS_BSY) && timeout > 0) {
        timeout--;
    }
}

int ata_init(void) {
    printf("Initializing ATA subsystem...\n");
    
    ata_reset_controller(ATA_PRIMARY_BASE);
    ata_reset_controller(ATA_SECONDARY_BASE);
    
    memset(ata_devices, 0, sizeof(ata_devices));
    
    ata_devices[0].base = ATA_PRIMARY_BASE;
    ata_devices[0].control = ATA_PRIMARY_BASE + ATA_REG_CONTROL;
    ata_devices[0].drive = 0;
    
    ata_devices[1].base = ATA_PRIMARY_BASE;
    ata_devices[1].control = ATA_PRIMARY_BASE + ATA_REG_CONTROL;
    ata_devices[1].drive = 1;
    
    ata_devices[2].base = ATA_SECONDARY_BASE;
    ata_devices[2].control = ATA_SECONDARY_BASE + ATA_REG_CONTROL;
    ata_devices[2].drive = 0;
    
    ata_devices[3].base = ATA_SECONDARY_BASE;
    ata_devices[3].control = ATA_SECONDARY_BASE + ATA_REG_CONTROL;
    ata_devices[3].drive = 1;
    
    int found_devices = 0;
    for (int i = 0; i < 4; i++) {
        printf("Checking ATA device %d...\n", i);
        if (ata_identify(&ata_devices[i]) == 0) {
            found_devices++;
        } else {
            printf("ATA device %d not found\n", i);
        }
    }
    
    printf("Found %d ATA device(s)\n", found_devices);
    return found_devices;
}

ata_device_t* get_ata_device(int drive) {
    if (drive >= 0 && drive < 2) {
        return &ata_devices[drive];
    }
    
    return NULL;
}


void debug_lba_registers(ata_device_t *dev, uint32_t lba, uint8_t count) {
    printf("LBA: 0x%X, Count: %d\n", lba, count);
    printf("Registers: ");
    printf("SECCOUNT: 0x%X, ", inb(dev->base + ATA_REG_SECCOUNT));
    printf("LBA_LOW: 0x%X, ", inb(dev->base + ATA_REG_LBA_LOW));
    printf("LBA_MID: 0x%X, ", inb(dev->base + ATA_REG_LBA_MID));
    printf("LBA_HIGH: 0x%X, ", inb(dev->base + ATA_REG_LBA_HIGH));
    printf("DRIVE: 0x%X\n", inb(dev->base + ATA_REG_DRIVE));
}

int ata_read_sectors_debug(ata_device_t *dev, uint32_t lba, uint8_t count, void *buffer) {
    printf("=== READ DEBUG ===\n");
    debug_lba_registers(dev, lba, count);
    
    // Вызов обычной функции чтения
    int result = ata_read_sectors(dev, lba, count, buffer);
    
    printf("Status: 0x%X, Error: 0x%X\n", 
           inb(dev->base + ATA_REG_STATUS),
           inb(dev->base + ATA_REG_ERROR));
    printf("=== END DEBUG ===\n");
    
    return result;
}