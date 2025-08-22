#include "dma.h"
#include <asm/io.h>

#define DMA_MASTER_BASE 0x00
#define DMA_SLAVE_BASE  0xC0

static const uint16_t dma_page_ports[DMA_CHANNEL_COUNT] = {
    0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A
};

static const uint16_t dma_addr_ports[DMA_CHANNEL_COUNT] = {
    0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC
};

static const uint16_t dma_count_ports[DMA_CHANNEL_COUNT] = {
    0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE
};

static const uint16_t dma_mask_ports[2] = {0x0A, 0xD4};
static const uint16_t dma_mode_ports[2] = {0x0B, 0xD6};
static const uint16_t dma_clear_ports[2] = {0x0C, 0xD8};
static const uint16_t dma_status_ports[2] = {0x08, 0xD0};

static inline bool is_valid_channel(dma_channel_t channel) {
    return channel < DMA_CHANNEL_COUNT;
}

static inline uint8_t get_controller_index(dma_channel_t channel) {
    return channel > DMA_CHANNEL_3 ? 1 : 0;
}

void dma_init(void) {
    for (dma_channel_t ch = DMA_CHANNEL_0; ch < DMA_CHANNEL_COUNT; ch++) {
        dma_mask_channel(ch);
    }
    dma_reset_flipflop(true);
}

bool dma_setup_channel(dma_channel_t channel, uint32_t phys_addr, uint16_t count, dma_mode_t mode) {
    if (!is_valid_channel(channel)) return false;

    const uint8_t ctrl_idx = get_controller_index(channel);
    const uint8_t chan_num = channel & 0x03;

    dma_mask_channel(channel);
    dma_reset_flipflop(ctrl_idx == 0);

    outb(dma_mode_ports[ctrl_idx], mode | chan_num);
    dma_reset_flipflop(ctrl_idx == 0);

    outb(dma_addr_ports[channel], (uint8_t)(phys_addr & 0xFF));
    outb(dma_addr_ports[channel], (uint8_t)((phys_addr >> 8) & 0xFF));

    outb(dma_page_ports[channel], (uint8_t)((phys_addr >> 16) & 0xFF));

    outb(dma_count_ports[channel], (uint8_t)(count & 0xFF));
    outb(dma_count_ports[channel], (uint8_t)((count >> 8) & 0xFF));

    dma_unmask_channel(channel);
    return true;
}

void dma_mask_channel(dma_channel_t channel) {
    if (!is_valid_channel(channel)) return;
    outb(dma_mask_ports[get_controller_index(channel)], 0x04 | (channel & 0x03));
}

void dma_unmask_channel(dma_channel_t channel) {
    if (!is_valid_channel(channel)) return;
    outb(dma_mask_ports[get_controller_index(channel)], channel & 0x03);
}

void dma_reset_flipflop(bool both_controllers) {
    outb(dma_clear_ports[0], 0xFF);
    if (both_controllers) {
        outb(dma_clear_ports[1], 0xFF);
    }
}

uint8_t dma_get_status(bool master_controller) {
    return inb(dma_status_ports[master_controller ? 0 : 1]);
}

bool dma_is_channel_active(dma_channel_t channel) {
    if (!is_valid_channel(channel)) return false;
    uint8_t status = dma_get_status(get_controller_index(channel) == 0);
    return (status & (1 << (channel & 0x03))) != 0;
}