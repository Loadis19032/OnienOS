#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DMA_CHANNEL_0,
    DMA_CHANNEL_1,
    DMA_CHANNEL_2,
    DMA_CHANNEL_3,
    DMA_CHANNEL_4,
    DMA_CHANNEL_5,
    DMA_CHANNEL_6,
    DMA_CHANNEL_7,
    DMA_CHANNEL_COUNT
} dma_channel_t;

typedef enum {
    DMA_MODE_READ,
    DMA_MODE_WRITE,
    DMA_MODE_VERIFY,
    DMA_MODE_AUTOINIT_READ = 0x45,
    DMA_MODE_AUTOINIT_WRITE = 0x49
} dma_mode_t;

void dma_init(void);
bool dma_setup_channel(dma_channel_t channel, uint32_t phys_addr, uint16_t count, dma_mode_t mode);
void dma_mask_channel(dma_channel_t channel);
void dma_unmask_channel(dma_channel_t channel);
void dma_reset_flipflop(bool both_controllers);
uint8_t dma_get_status(bool master_controller);
bool dma_is_channel_active(dma_channel_t channel);

#endif