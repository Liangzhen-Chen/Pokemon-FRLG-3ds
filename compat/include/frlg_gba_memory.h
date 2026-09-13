#ifndef FRLG_GBA_MEMORY_H
#define FRLG_GBA_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    FRLG_GBA_EWRAM_BASE = 0x02000000,
    FRLG_GBA_EWRAM_SIZE = 0x00040000,
    FRLG_GBA_IWRAM_BASE = 0x03000000,
    FRLG_GBA_IWRAM_SIZE = 0x00008000,
    FRLG_GBA_IO_BASE = 0x04000000,
    FRLG_GBA_IO_SIZE = 0x00000400,
    FRLG_GBA_PALETTE_BASE = 0x05000000,
    FRLG_GBA_PALETTE_SIZE = 0x00000400,
    FRLG_GBA_VRAM_BASE = 0x06000000,
    FRLG_GBA_VRAM_SIZE = 0x00018000,
    FRLG_GBA_OAM_BASE = 0x07000000,
    FRLG_GBA_OAM_SIZE = 0x00000400
};

typedef struct FrlgGbaMemory {
    uint8_t ewram[FRLG_GBA_EWRAM_SIZE];
    uint8_t iwram[FRLG_GBA_IWRAM_SIZE];
    uint8_t io[FRLG_GBA_IO_SIZE];
    uint8_t palette[FRLG_GBA_PALETTE_SIZE];
    uint8_t vram[FRLG_GBA_VRAM_SIZE];
    uint8_t oam[FRLG_GBA_OAM_SIZE];
} FrlgGbaMemory;

void frlg_gba_memory_reset(FrlgGbaMemory *memory);
bool frlg_gba_memory_map(FrlgGbaMemory *memory, uint32_t address, size_t size, uint8_t **mapped);
bool frlg_gba_memory_map_const(const FrlgGbaMemory *memory, uint32_t address, size_t size, const uint8_t **mapped);
bool frlg_gba_memory_read8(const FrlgGbaMemory *memory, uint32_t address, uint8_t *value);
bool frlg_gba_memory_read16(const FrlgGbaMemory *memory, uint32_t address, uint16_t *value);
bool frlg_gba_memory_read32(const FrlgGbaMemory *memory, uint32_t address, uint32_t *value);
bool frlg_gba_memory_write8(FrlgGbaMemory *memory, uint32_t address, uint8_t value);
bool frlg_gba_memory_write16(FrlgGbaMemory *memory, uint32_t address, uint16_t value);
bool frlg_gba_memory_write32(FrlgGbaMemory *memory, uint32_t address, uint32_t value);

#endif
