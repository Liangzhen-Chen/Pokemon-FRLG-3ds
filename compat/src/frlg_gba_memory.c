#include "frlg_gba_memory.h"

#include <string.h>

static bool map_region(uint8_t *region, uint32_t base, size_t region_size,
                       uint32_t address, size_t size, uint8_t **mapped)
{
    size_t offset;

    if (address < base || size == 0 || mapped == NULL)
        return false;

    offset = (size_t)(address - base);
    if (offset >= region_size || size > region_size - offset)
        return false;

    *mapped = region + offset;
    return true;
}

void frlg_gba_memory_reset(FrlgGbaMemory *memory)
{
    if (memory != NULL)
        memset(memory, 0, sizeof(*memory));
}

bool frlg_gba_memory_map(FrlgGbaMemory *memory, uint32_t address, size_t size, uint8_t **mapped)
{
    if (memory == NULL)
        return false;

    if (map_region(memory->ewram, FRLG_GBA_EWRAM_BASE, sizeof(memory->ewram), address, size, mapped))
        return true;
    if (map_region(memory->iwram, FRLG_GBA_IWRAM_BASE, sizeof(memory->iwram), address, size, mapped))
        return true;
    if (map_region(memory->io, FRLG_GBA_IO_BASE, sizeof(memory->io), address, size, mapped))
        return true;
    if (map_region(memory->palette, FRLG_GBA_PALETTE_BASE, sizeof(memory->palette), address, size, mapped))
        return true;
    if (map_region(memory->vram, FRLG_GBA_VRAM_BASE, sizeof(memory->vram), address, size, mapped))
        return true;
    return map_region(memory->oam, FRLG_GBA_OAM_BASE, sizeof(memory->oam), address, size, mapped);
}

bool frlg_gba_memory_map_const(const FrlgGbaMemory *memory, uint32_t address, size_t size, const uint8_t **mapped)
{
    uint8_t *mutable_mapped;

    if (mapped == NULL || !frlg_gba_memory_map((FrlgGbaMemory *)memory, address, size, &mutable_mapped))
        return false;
    *mapped = mutable_mapped;
    return true;
}

bool frlg_gba_memory_read8(const FrlgGbaMemory *memory, uint32_t address, uint8_t *value)
{
    const uint8_t *data;
    if (value == NULL || !frlg_gba_memory_map_const(memory, address, 1, &data))
        return false;
    *value = data[0];
    return true;
}

bool frlg_gba_memory_read16(const FrlgGbaMemory *memory, uint32_t address, uint16_t *value)
{
    const uint8_t *data;
    if (value == NULL || !frlg_gba_memory_map_const(memory, address, 2, &data))
        return false;
    *value = (uint16_t)data[0] | (uint16_t)((uint16_t)data[1] << 8);
    return true;
}

bool frlg_gba_memory_read32(const FrlgGbaMemory *memory, uint32_t address, uint32_t *value)
{
    const uint8_t *data;
    if (value == NULL || !frlg_gba_memory_map_const(memory, address, 4, &data))
        return false;
    *value = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
             ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    return true;
}

bool frlg_gba_memory_write8(FrlgGbaMemory *memory, uint32_t address, uint8_t value)
{
    uint8_t *data;
    if (!frlg_gba_memory_map(memory, address, 1, &data))
        return false;
    data[0] = value;
    return true;
}

bool frlg_gba_memory_write16(FrlgGbaMemory *memory, uint32_t address, uint16_t value)
{
    uint8_t *data;
    if (!frlg_gba_memory_map(memory, address, 2, &data))
        return false;
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    return true;
}

bool frlg_gba_memory_write32(FrlgGbaMemory *memory, uint32_t address, uint32_t value)
{
    uint8_t *data;
    if (!frlg_gba_memory_map(memory, address, 4, &data))
        return false;
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
    return true;
}
