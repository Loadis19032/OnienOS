#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stdbool.h>

#define PCI_MAX_BUSES     256
#define PCI_MAX_DEVICES   32
#define PCI_MAX_FUNCTIONS 8

#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02
#define PCI_COMMAND              0x04
#define PCI_STATUS               0x06
#define PCI_REVISION_ID          0x08
#define PCI_PROG_IF              0x09
#define PCI_SUBCLASS             0x0A
#define PCI_CLASS                0x0B
#define PCI_CACHE_LINE_SIZE      0x0C
#define PCI_LATENCY_TIMER        0x0D
#define PCI_HEADER_TYPE          0x0E
#define PCI_BIST                0x0F
#define PCI_BAR0                0x10
#define PCI_BAR1                0x14
#define PCI_BAR2                0x18
#define PCI_BAR3                0x1C
#define PCI_BAR4                0x20
#define PCI_BAR5                0x24
#define PCI_CAPABILITIES_PTR    0x34
#define PCI_INTERRUPT_LINE       0x3C
#define PCI_INTERRUPT_PIN        0x3D
#define PCI_SECONDARY_BUS        0x19

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2
#define PCI_HEADER_TYPE_MULTI_FUNC 0x80

#define PCI_COMMAND_IO           0x1
#define PCI_COMMAND_MEMORY       0x2
#define PCI_COMMAND_MASTER       0x4
#define PCI_COMMAND_SPECIAL      0x8
#define PCI_COMMAND_INVALIDATE   0x10
#define PCI_COMMAND_VGA_PALETTE  0x20
#define PCI_COMMAND_PARITY       0x40
#define PCI_COMMAND_WAIT         0x80
#define PCI_COMMAND_SERR         0x100
#define PCI_COMMAND_FAST_BACK    0x200
#define PCI_COMMAND_INTERRUPT    0x400

#define PCI_STATUS_CAPABILITIES  0x10

#define PCI_CAP_ID_PM           0x01
#define PCI_CAP_ID_AGP          0x02
#define PCI_CAP_ID_VPD          0x03
#define PCI_CAP_ID_SLOTID       0x04
#define PCI_CAP_ID_MSI          0x05
#define PCI_CAP_ID_CHSWP        0x06
#define PCI_CAP_ID_PCIX         0x07
#define PCI_CAP_ID_HT           0x08
#define PCI_CAP_ID_VNDR         0x09
#define PCI_CAP_ID_DBG          0x0A
#define PCI_CAP_ID_CCRC         0x0B
#define PCI_CAP_ID_SHPC         0x0C
#define PCI_CAP_ID_SSVID        0x0D
#define PCI_CAP_ID_AGP3         0x0E
#define PCI_CAP_ID_SECDEV       0x0F
#define PCI_CAP_ID_EXP          0x10
#define PCI_CAP_ID_MSIX         0x11
#define PCI_CAP_ID_SATA         0x12
#define PCI_CAP_ID_AF           0x13

#define PCI_EXP_FLAGS           0x02  
#define PCI_EXP_FLAGS_VERS      0x000f
#define PCI_EXP_FLAGS_TYPE      0x00f0
#define PCI_EXP_TYPE_ENDPOINT   0x0  
#define PCI_EXP_TYPE_LEG_END    0x1  
#define PCI_EXP_TYPE_ROOT_PORT  0x4  
#define PCI_EXP_TYPE_UPSTREAM   0x5  
#define PCI_EXP_TYPE_DOWNSTREAM 0x6  
#define PCI_EXP_TYPE_PCI_BRIDGE 0x7  
#define PCI_EXP_FLAGS_SLOT      0x0100
#define PCI_EXP_FLAGS_IRQ       0x3e00

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t header_type;
    uint32_t bars[6];
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    bool has_capabilities;
    uint8_t capabilities_ptr;
} pci_device_t;

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
} pci_location_t;

uint32_t pci_read_config32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t pci_read_config16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint8_t pci_read_config8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
void pci_write_config16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
void pci_write_config8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function);
uint16_t pci_get_device_id(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_class_code(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_subclass(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_prog_if(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_revision_id(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_interrupt_line(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_interrupt_pin(uint8_t bus, uint8_t device, uint8_t function);
uint16_t pci_get_subsystem_vendor_id(uint8_t bus, uint8_t device, uint8_t function);
uint16_t pci_get_subsystem_id(uint8_t bus, uint8_t device, uint8_t function);

bool pci_device_has_capabilities(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_capabilities_ptr(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_find_capability(uint8_t bus, uint8_t device, uint8_t function, uint8_t cap_id);

void pci_enable_bus_mastering(pci_device_t* dev);
void pci_enable_memory_space(pci_device_t* dev);
void pci_enable_io_space(pci_device_t* dev);
void pci_enable_interrupts(pci_device_t* dev);
void pci_disable_interrupts(pci_device_t* dev);

uint32_t pci_get_bar(pci_device_t* dev, uint8_t bar_num);
uint32_t pci_get_bar_size(pci_device_t* dev, uint8_t bar_num, bool io_space);
bool pci_is_bar_io(pci_device_t* dev, uint8_t bar_num);
bool pci_is_bar_64bit(pci_device_t* dev, uint8_t bar_num);
bool pci_is_bar_prefetchable(pci_device_t* dev, uint8_t bar_num);

void pci_scan_bus(uint8_t bus);
void pci_scan_device(uint8_t bus, uint8_t device);
void pci_scan_function(uint8_t bus, uint8_t device, uint8_t function);
void pci_scan_all_buses(void);

pci_device_t* pci_get_device(uint16_t vendor_id, uint16_t device_id);
pci_device_t* pci_get_device_by_class(uint8_t class_code, uint8_t subclass, uint8_t prog_if);

void pci_init(void);

#endif // PCI_H