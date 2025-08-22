#include "pci.h"
#include <asm/io.h>
#include <stdio.h>
#include "../../kernel/mm/mem.h"
#include <string.h>

#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

static pci_device_t* pci_devices[256];
static uint32_t pci_device_count = 0;

static uint32_t pci_make_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return (uint32_t)((bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC) | 0x80000000);
}

static pci_device_t* pci_add_device(uint8_t bus, uint8_t device, uint8_t function) {
    pci_device_t* dev = (pci_device_t*)kmalloc(sizeof(pci_device_t));
    if (!dev) return NULL;
    
    memset(dev, 0, sizeof(pci_device_t));
    
    dev->bus = bus;
    dev->device = device;
    dev->function = function;
    dev->vendor_id = pci_get_vendor_id(bus, device, function);
    dev->device_id = pci_get_device_id(bus, device, function);
    dev->class_code = pci_get_class_code(bus, device, function);
    dev->subclass = pci_get_subclass(bus, device, function);
    dev->prog_if = pci_get_prog_if(bus, device, function);
    dev->revision_id = pci_get_revision_id(bus, device, function);
    dev->header_type = pci_get_header_type(bus, device, function);
    dev->interrupt_line = pci_get_interrupt_line(bus, device, function);
    dev->interrupt_pin = pci_get_interrupt_pin(bus, device, function);
    dev->subsystem_vendor_id = pci_get_subsystem_vendor_id(bus, device, function);
    dev->subsystem_id = pci_get_subsystem_id(bus, device, function);
    
    for (int i = 0; i < 6; i++) {
        dev->bars[i] = pci_read_config32(bus, device, function, PCI_BAR0 + (i * 4));
    }
    
    dev->has_capabilities = pci_device_has_capabilities(bus, device, function);
    if (dev->has_capabilities) {
        dev->capabilities_ptr = pci_get_capabilities_ptr(bus, device, function);
    }
    
    pci_devices[pci_device_count++] = dev;
    return dev;
}

uint32_t pci_read_config32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_make_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read_config16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_make_address(bus, device, function, offset));
    return (uint16_t)(inl(PCI_CONFIG_DATA) >> ((offset & 2) ? 16 : 0)) & 0xFFFF;
}

uint8_t pci_read_config8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_make_address(bus, device, function, offset));
    return (uint8_t)(inl(PCI_CONFIG_DATA) >> ((offset & 3) * 8)) & 0xFF;
}

void pci_write_config32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    outl(PCI_CONFIG_ADDRESS, pci_make_address(bus, device, function, offset));
    outl(PCI_CONFIG_DATA, value);
}

void pci_write_config16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value) {
    uint32_t tmp = pci_read_config32(bus, device, function, offset & ~3);
    if (offset & 2) {
        tmp = (tmp & 0x0000FFFF) | ((uint32_t)value << 16);
    } else {
        tmp = (tmp & 0xFFFF0000) | value;
    }
    pci_write_config32(bus, device, function, offset & ~3, tmp);
}

void pci_write_config8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value) {
    uint32_t tmp = pci_read_config32(bus, device, function, offset & ~3);
    tmp &= ~(0xFF << ((offset & 3) * 8));
    tmp |= (uint32_t)value << ((offset & 3) * 8);
    pci_write_config32(bus, device, function, offset & ~3, tmp);
}

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config16(bus, device, function, PCI_VENDOR_ID);
}

uint16_t pci_get_device_id(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config16(bus, device, function, PCI_DEVICE_ID);
}

uint8_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_HEADER_TYPE);
}

uint8_t pci_get_class_code(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_CLASS);
}

uint8_t pci_get_subclass(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_SUBCLASS);
}

uint8_t pci_get_prog_if(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_PROG_IF);
}

uint8_t pci_get_revision_id(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_REVISION_ID);
}

uint8_t pci_get_interrupt_line(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_INTERRUPT_LINE);
}

uint8_t pci_get_interrupt_pin(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_INTERRUPT_PIN);
}

uint16_t pci_get_subsystem_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config16(bus, device, function, 0x2C);
}

uint16_t pci_get_subsystem_id(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config16(bus, device, function, 0x2E);
}

bool pci_device_has_capabilities(uint8_t bus, uint8_t device, uint8_t function) {
    return (pci_read_config16(bus, device, function, PCI_STATUS) & PCI_STATUS_CAPABILITIES) != 0;
}

uint8_t pci_get_capabilities_ptr(uint8_t bus, uint8_t device, uint8_t function) {
    return pci_read_config8(bus, device, function, PCI_CAPABILITIES_PTR);
}

uint8_t pci_find_capability(uint8_t bus, uint8_t device, uint8_t function, uint8_t cap_id) {
    if (!pci_device_has_capabilities(bus, device, function))
        return 0;
    
    uint8_t ptr = pci_get_capabilities_ptr(bus, device, function);
    while (ptr != 0) {
        uint8_t id = pci_read_config8(bus, device, function, ptr);
        if (id == cap_id)
            return ptr;
        
        ptr = pci_read_config8(bus, device, function, ptr + 1);
    }
    
    return 0;
}

void pci_enable_bus_mastering(pci_device_t* dev) {
    uint16_t command = pci_read_config16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    command |= PCI_COMMAND_MASTER;
    pci_write_config16(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
}

void pci_enable_memory_space(pci_device_t* dev) {
    uint16_t command = pci_read_config16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    command |= PCI_COMMAND_MEMORY;
    pci_write_config16(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
}

void pci_enable_io_space(pci_device_t* dev) {
    uint16_t command = pci_read_config16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    command |= PCI_COMMAND_IO;
    pci_write_config16(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
}

void pci_enable_interrupts(pci_device_t* dev) {
    uint16_t command = pci_read_config16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    command &= ~PCI_COMMAND_INTERRUPT;
    pci_write_config16(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
}

void pci_disable_interrupts(pci_device_t* dev) {
    uint16_t command = pci_read_config16(dev->bus, dev->device, dev->function, PCI_COMMAND);
    command |= PCI_COMMAND_INTERRUPT;
    pci_write_config16(dev->bus, dev->device, dev->function, PCI_COMMAND, command);
}

uint32_t pci_get_bar(pci_device_t* dev, uint8_t bar_num) {
    if (bar_num >= 6) return 0;
    return dev->bars[bar_num];
}

uint32_t pci_get_bar_size(pci_device_t* dev, uint8_t bar_num, bool io_space) {
    if (bar_num >= 6) return 0;
    
    uint32_t original = pci_read_config32(dev->bus, dev->device, dev->function, PCI_BAR0 + bar_num * 4);
    
    pci_write_config32(dev->bus, dev->device, dev->function, PCI_BAR0 + bar_num * 4, 0xFFFFFFFF);
    
    uint32_t size = pci_read_config32(dev->bus, dev->device, dev->function, PCI_BAR0 + bar_num * 4);
    
    pci_write_config32(dev->bus, dev->device, dev->function, PCI_BAR0 + bar_num * 4, original);
    
    if (io_space) {
        size &= 0xFFFFFFFC;
        return (~size) + 1;
    } else {
        size &= 0xFFFFFFF0;
        return (~size) + 1;
    }
}

bool pci_is_bar_io(pci_device_t* dev, uint8_t bar_num) {
    if (bar_num >= 6) return false;
    return (dev->bars[bar_num] & 0x01) == 0x01;
}

bool pci_is_bar_64bit(pci_device_t* dev, uint8_t bar_num) {
    if (bar_num >= 6) return false;
    if (pci_is_bar_io(dev, bar_num)) return false;
    return (dev->bars[bar_num] & 0x06) == 0x04;
}

bool pci_is_bar_prefetchable(pci_device_t* dev, uint8_t bar_num) {
    if (bar_num >= 6) return false;
    if (pci_is_bar_io(dev, bar_num)) return false;
    return (dev->bars[bar_num] & 0x08) == 0x08;
}

void pci_scan_function(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id = pci_get_vendor_id(bus, device, function);
    if (vendor_id == 0xFFFF) return;
    
    pci_device_t* dev = pci_add_device(bus, device, function);
    if (!dev) return;
    
    uint8_t class_code = pci_get_class_code(bus, device, function);
    uint8_t subclass = pci_get_subclass(bus, device, function);
    
    printf("PCI: %02x:%02x.%x %04x:%04x Class: %02x:%02x.%02x\n",
           bus, device, function, vendor_id, pci_get_device_id(bus, device, function),
           class_code, subclass, pci_get_prog_if(bus, device, function));
    
    if (class_code == 0x06 && subclass == 0x04) {
        uint8_t secondary_bus = pci_read_config8(bus, device, function, PCI_SECONDARY_BUS);
        pci_scan_bus(secondary_bus);
    }
}

void pci_scan_device(uint8_t bus, uint8_t device) {
    uint16_t vendor_id = pci_get_vendor_id(bus, device, 0);
    if (vendor_id == 0xFFFF) return;
    
    pci_scan_function(bus, device, 0);
    
    uint8_t header_type = pci_get_header_type(bus, device, 0);
    if ((header_type & PCI_HEADER_TYPE_MULTI_FUNC) != 0) {
        for (uint8_t function = 1; function < 8; function++) {
            if (pci_get_vendor_id(bus, device, function) != 0xFFFF) {
                pci_scan_function(bus, device, function);
            }
        }
    }
}

void pci_scan_bus(uint8_t bus) {
    for (uint8_t device = 0; device < PCI_MAX_DEVICES; device++) {
        pci_scan_device(bus, device);
    }
}

void pci_scan_all_buses(void) {
    uint8_t header_type = pci_get_header_type(0, 0, 0);
    
    if ((header_type & PCI_HEADER_TYPE_MULTI_FUNC) == 0) {
        pci_scan_bus(0);
    } else {
        for (uint8_t function = 0; function < 8; function++) {
            if (pci_get_vendor_id(0, 0, function) != 0xFFFF) {
                pci_scan_bus(function);
            } else {
                break;
            }
        }
    }
}

pci_device_t* pci_get_device(uint16_t vendor_id, uint16_t device_id) {
    for (uint32_t i = 0; i < pci_device_count; i++) {
        if (pci_devices[i]->vendor_id == vendor_id && pci_devices[i]->device_id == device_id) {
            return pci_devices[i];
        }
    }
    return NULL;
}

pci_device_t* pci_get_device_by_class(uint8_t class_code, uint8_t subclass, uint8_t prog_if) {
    for (uint32_t i = 0; i < pci_device_count; i++) {
        if (pci_devices[i]->class_code == class_code && 
            pci_devices[i]->subclass == subclass && 
            (prog_if == 0xFF || pci_devices[i]->prog_if == prog_if)) {
            return pci_devices[i];
        }
    }
    return NULL;
}

void pci_init(void) {
    printf("Initializing PCI subsystem...\n");
    memset(pci_devices, 0, sizeof(pci_devices));
    pci_device_count = 0;
    
    pci_scan_all_buses();
    
    printf("PCI: Found %d devices\n", pci_device_count);
}