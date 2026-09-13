#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "frlg_gba_memory.h"

static FrlgGbaMemory memory;

void run_gba_memory_tests(void)
{
    const uint8_t *read_ptr = NULL;
    uint8_t *write_ptr = NULL;
    uint8_t value8 = 0xff;
    uint16_t value16 = 0;
    uint32_t value32 = 0;

    frlg_gba_memory_reset(&memory);
    assert(memory.ewram[0] == 0);
    assert(memory.vram[FRLG_GBA_VRAM_SIZE - 1] == 0);

    assert(frlg_gba_memory_write32(&memory, FRLG_GBA_EWRAM_BASE, UINT32_C(0x78563412)));
    assert(memory.ewram[0] == 0x12);
    assert(memory.ewram[1] == 0x34);
    assert(frlg_gba_memory_read32(&memory, FRLG_GBA_EWRAM_BASE, &value32));
    assert(value32 == UINT32_C(0x78563412));

    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 2, UINT16_C(0xabcd)));
    assert(frlg_gba_memory_read16(&memory, FRLG_GBA_IO_BASE + 2, &value16));
    assert(value16 == UINT16_C(0xabcd));

    assert(frlg_gba_memory_write8(&memory, FRLG_GBA_OAM_BASE + FRLG_GBA_OAM_SIZE - 1, 0x5a));
    assert(frlg_gba_memory_read8(&memory, FRLG_GBA_OAM_BASE + FRLG_GBA_OAM_SIZE - 1, &value8));
    assert(value8 == 0x5a);

    assert(frlg_gba_memory_map(&memory, FRLG_GBA_PALETTE_BASE, FRLG_GBA_PALETTE_SIZE, &write_ptr));
    assert(write_ptr == memory.palette);
    assert(frlg_gba_memory_map_const(&memory, FRLG_GBA_VRAM_BASE, FRLG_GBA_VRAM_SIZE, &read_ptr));
    assert(read_ptr == memory.vram);

    assert(!frlg_gba_memory_map(&memory, FRLG_GBA_EWRAM_BASE, 0, &write_ptr));
    assert(!frlg_gba_memory_map(&memory, FRLG_GBA_EWRAM_BASE + FRLG_GBA_EWRAM_SIZE - 1, 2, &write_ptr));
    assert(!frlg_gba_memory_read8(&memory, UINT32_C(0x08000000), &value8));
    assert(!frlg_gba_memory_write32(&memory, UINT32_MAX - 1, 1));
    assert(!frlg_gba_memory_read8(NULL, FRLG_GBA_EWRAM_BASE, &value8));
    assert(!frlg_gba_memory_read8(&memory, FRLG_GBA_EWRAM_BASE, NULL));
}
