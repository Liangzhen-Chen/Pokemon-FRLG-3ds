#ifndef FRLG_GBA_DISPLAY_H
#define FRLG_GBA_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#include "frlg_gba_memory.h"

enum {
    FRLG_GBA_REG_DISPCNT = FRLG_GBA_IO_BASE + 0x0000,
    FRLG_GBA_REG_DISPSTAT = FRLG_GBA_IO_BASE + 0x0004,
    FRLG_GBA_REG_VCOUNT = FRLG_GBA_IO_BASE + 0x0006,
    FRLG_GBA_REG_BG0CNT = FRLG_GBA_IO_BASE + 0x0008,
    FRLG_GBA_REG_BG1CNT = FRLG_GBA_IO_BASE + 0x000a,
    FRLG_GBA_REG_BG2CNT = FRLG_GBA_IO_BASE + 0x000c,
    FRLG_GBA_REG_BG3CNT = FRLG_GBA_IO_BASE + 0x000e,
    FRLG_GBA_REG_BG0HOFS = FRLG_GBA_IO_BASE + 0x0010,
    FRLG_GBA_REG_BG0VOFS = FRLG_GBA_IO_BASE + 0x0012
};

enum {
    FRLG_GBA_DISPCNT_MODE_MASK = 0x0007,
    FRLG_GBA_DISPCNT_FORCED_BLANK = 0x0080,
    FRLG_GBA_DISPCNT_BG0 = 0x0100,
    FRLG_GBA_DISPCNT_BG1 = 0x0200,
    FRLG_GBA_DISPCNT_BG2 = 0x0400,
    FRLG_GBA_DISPCNT_BG3 = 0x0800,
    FRLG_GBA_DISPCNT_OBJ = 0x1000,
    FRLG_GBA_DISPCNT_LAYER_MASK = 0x1f00
};

typedef struct FrlgGbaDisplaySnapshot {
    uint16_t control;
    uint16_t status;
    uint16_t vcount;
    uint16_t background_control[4];
    uint16_t background_x[4];
    uint16_t background_y[4];
    uint8_t mode;
    uint8_t layer_mask;
    bool forced_blank;
} FrlgGbaDisplaySnapshot;

bool frlg_gba_display_set_control(FrlgGbaMemory *memory, uint16_t value);
bool frlg_gba_display_set_background_control(FrlgGbaMemory *memory, unsigned int background, uint16_t value);
bool frlg_gba_display_snapshot(const FrlgGbaMemory *memory, FrlgGbaDisplaySnapshot *snapshot);

#endif
