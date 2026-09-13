#ifndef FRLG_GBA_DMA_H
#define FRLG_GBA_DMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "frlg_gba_memory.h"

typedef enum FrlgGbaDmaAddressMode {
    FRLG_GBA_DMA_INCREMENT = 0,
    FRLG_GBA_DMA_FIXED = 1
} FrlgGbaDmaAddressMode;

typedef struct FrlgGbaDmaCopy {
    uint32_t source;
    uint32_t destination;
    size_t units;
    size_t width;
    FrlgGbaDmaAddressMode source_mode;
    FrlgGbaDmaAddressMode destination_mode;
} FrlgGbaDmaCopy;

bool frlg_gba_dma_copy(FrlgGbaMemory *memory, const FrlgGbaDmaCopy *copy);

#endif
