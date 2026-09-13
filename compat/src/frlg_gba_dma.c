#include "frlg_gba_dma.h"

#include <limits.h>

static bool valid_mode(FrlgGbaDmaAddressMode mode)
{
    return mode == FRLG_GBA_DMA_INCREMENT || mode == FRLG_GBA_DMA_FIXED;
}

static bool validate_range(FrlgGbaMemory *memory, uint32_t address, size_t units,
                           size_t width, FrlgGbaDmaAddressMode mode)
{
    uint8_t *mapped;
    size_t bytes;

    if (mode == FRLG_GBA_DMA_FIXED)
        return frlg_gba_memory_map(memory, address, width, &mapped);

    if (units > SIZE_MAX / width)
        return false;
    bytes = units * width;
    if (bytes > UINT32_MAX || address > UINT32_MAX - (uint32_t)(bytes - 1))
        return false;
    return frlg_gba_memory_map(memory, address, bytes, &mapped);
}

bool frlg_gba_dma_copy(FrlgGbaMemory *memory, const FrlgGbaDmaCopy *copy)
{
    uint32_t source;
    uint32_t destination;
    size_t unit;

    if (memory == NULL || copy == NULL || copy->units == 0 ||
        (copy->width != 2 && copy->width != 4) ||
        !valid_mode(copy->source_mode) || !valid_mode(copy->destination_mode) ||
        !validate_range(memory, copy->source, copy->units, copy->width, copy->source_mode) ||
        !validate_range(memory, copy->destination, copy->units, copy->width, copy->destination_mode))
        return false;

    source = copy->source;
    destination = copy->destination;
    for (unit = 0; unit < copy->units; unit++)
    {
        if (copy->width == 2)
        {
            uint16_t value;
            if (!frlg_gba_memory_read16(memory, source, &value) ||
                !frlg_gba_memory_write16(memory, destination, value))
                return false;
        }
        else
        {
            uint32_t value;
            if (!frlg_gba_memory_read32(memory, source, &value) ||
                !frlg_gba_memory_write32(memory, destination, value))
                return false;
        }

        if (copy->source_mode == FRLG_GBA_DMA_INCREMENT)
            source += (uint32_t)copy->width;
        if (copy->destination_mode == FRLG_GBA_DMA_INCREMENT)
            destination += (uint32_t)copy->width;
    }
    return true;
}
