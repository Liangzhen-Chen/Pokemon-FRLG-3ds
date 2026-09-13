#include <assert.h>
#include <stdint.h>

#include "frlg_gba_display.h"
#include "frlg_gba_dma.h"

static FrlgGbaMemory memory;

static void test_display_snapshot(void)
{
    FrlgGbaDisplaySnapshot snapshot;

    frlg_gba_memory_reset(&memory);
    assert(frlg_gba_display_set_control(&memory,
        1 | FRLG_GBA_DISPCNT_BG0 | FRLG_GBA_DISPCNT_BG2 | FRLG_GBA_DISPCNT_OBJ));
    assert(frlg_gba_display_set_background_control(&memory, 0, UINT16_C(0x0083)));
    assert(frlg_gba_display_set_background_control(&memory, 3, UINT16_C(0x1f0c)));
    assert(!frlg_gba_display_set_background_control(&memory, 4, 0));

    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_REG_DISPSTAT, UINT16_C(0x0001)));
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_REG_VCOUNT, UINT16_C(159)));
    assert(frlg_gba_display_snapshot(&memory, &snapshot));
    assert(snapshot.mode == 1);
    assert(snapshot.layer_mask == 0x15);
    assert(!snapshot.forced_blank);
    assert(snapshot.status == 1);
    assert(snapshot.vcount == 159);
    assert(snapshot.background_control[0] == UINT16_C(0x0083));
    assert(snapshot.background_control[3] == UINT16_C(0x1f0c));

    assert(frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_FORCED_BLANK));
    assert(frlg_gba_display_snapshot(&memory, &snapshot));
    assert(snapshot.forced_blank);
    assert(!frlg_gba_display_snapshot(NULL, &snapshot));
    assert(!frlg_gba_display_snapshot(&memory, NULL));
}

static void test_dma_copy(void)
{
    FrlgGbaDmaCopy copy = {
        .source = FRLG_GBA_EWRAM_BASE,
        .destination = FRLG_GBA_VRAM_BASE,
        .units = 4,
        .width = 2,
        .source_mode = FRLG_GBA_DMA_INCREMENT,
        .destination_mode = FRLG_GBA_DMA_INCREMENT,
    };
    uint16_t value16;
    uint32_t value32;

    frlg_gba_memory_reset(&memory);
    for (unsigned int i = 0; i < 4; i++)
        assert(frlg_gba_memory_write16(&memory, FRLG_GBA_EWRAM_BASE + i * 2, (uint16_t)(0x1100 + i)));
    assert(frlg_gba_dma_copy(&memory, &copy));
    for (unsigned int i = 0; i < 4; i++)
    {
        assert(frlg_gba_memory_read16(&memory, FRLG_GBA_VRAM_BASE + i * 2, &value16));
        assert(value16 == (uint16_t)(0x1100 + i));
    }

    assert(frlg_gba_memory_write32(&memory, FRLG_GBA_IWRAM_BASE, UINT32_C(0x89abcdef)));
    copy.source = FRLG_GBA_IWRAM_BASE;
    copy.destination = FRLG_GBA_PALETTE_BASE;
    copy.units = 3;
    copy.width = 4;
    copy.source_mode = FRLG_GBA_DMA_FIXED;
    assert(frlg_gba_dma_copy(&memory, &copy));
    assert(frlg_gba_memory_read32(&memory, FRLG_GBA_PALETTE_BASE + 8, &value32));
    assert(value32 == UINT32_C(0x89abcdef));

    copy.source = FRLG_GBA_EWRAM_BASE + FRLG_GBA_EWRAM_SIZE - 2;
    copy.destination = FRLG_GBA_OAM_BASE;
    copy.units = 2;
    copy.width = 2;
    copy.source_mode = FRLG_GBA_DMA_INCREMENT;
    assert(!frlg_gba_dma_copy(&memory, &copy));
    assert(frlg_gba_memory_read16(&memory, FRLG_GBA_OAM_BASE, &value16));
    assert(value16 == 0);

    copy.source = FRLG_GBA_EWRAM_BASE;
    copy.destination = FRLG_GBA_VRAM_BASE;
    copy.units = 0;
    assert(!frlg_gba_dma_copy(&memory, &copy));
    copy.units = 1;
    copy.width = 1;
    assert(!frlg_gba_dma_copy(&memory, &copy));
    copy.width = 2;
    copy.source_mode = (FrlgGbaDmaAddressMode)99;
    assert(!frlg_gba_dma_copy(&memory, &copy));
    assert(!frlg_gba_dma_copy(NULL, &copy));
    assert(!frlg_gba_dma_copy(&memory, NULL));
}

void run_gba_io_dma_tests(void)
{
    test_display_snapshot();
    test_dma_copy();
}
