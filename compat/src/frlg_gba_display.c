#include "frlg_gba_display.h"

bool frlg_gba_display_set_control(FrlgGbaMemory *memory, uint16_t value)
{
    return frlg_gba_memory_write16(memory, FRLG_GBA_REG_DISPCNT, value);
}

bool frlg_gba_display_set_background_control(FrlgGbaMemory *memory, unsigned int background, uint16_t value)
{
    if (background >= 4)
        return false;
    return frlg_gba_memory_write16(memory, FRLG_GBA_REG_BG0CNT + (uint32_t)(background * 2), value);
}

bool frlg_gba_display_snapshot(const FrlgGbaMemory *memory, FrlgGbaDisplaySnapshot *snapshot)
{
    unsigned int background;

    if (snapshot == NULL ||
        !frlg_gba_memory_read16(memory, FRLG_GBA_REG_DISPCNT, &snapshot->control) ||
        !frlg_gba_memory_read16(memory, FRLG_GBA_REG_DISPSTAT, &snapshot->status) ||
        !frlg_gba_memory_read16(memory, FRLG_GBA_REG_VCOUNT, &snapshot->vcount))
        return false;

    for (background = 0; background < 4; background++)
    {
        if (!frlg_gba_memory_read16(memory,
                FRLG_GBA_REG_BG0CNT + (uint32_t)(background * 2),
                &snapshot->background_control[background]) ||
            !frlg_gba_memory_read16(memory, FRLG_GBA_REG_BG0HOFS + background * 4,
                &snapshot->background_x[background]) ||
            !frlg_gba_memory_read16(memory, FRLG_GBA_REG_BG0VOFS + background * 4,
                &snapshot->background_y[background]))
            return false;
        snapshot->background_x[background] &= 0x1ff;
        snapshot->background_y[background] &= 0x1ff;
    }

    snapshot->mode = (uint8_t)(snapshot->control & FRLG_GBA_DISPCNT_MODE_MASK);
    snapshot->layer_mask = (uint8_t)((snapshot->control & FRLG_GBA_DISPCNT_LAYER_MASK) >> 8);
    snapshot->forced_blank = (snapshot->control & FRLG_GBA_DISPCNT_FORCED_BLANK) != 0;
    return true;
}
