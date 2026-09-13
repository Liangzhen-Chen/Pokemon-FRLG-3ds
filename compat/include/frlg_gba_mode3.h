#ifndef FRLG_GBA_MODE3_H
#define FRLG_GBA_MODE3_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "frlg_gba_display.h"
#include "frlg_gba_memory.h"

enum {
    FRLG_GBA_SCREEN_WIDTH = 240,
    FRLG_GBA_SCREEN_HEIGHT = 160,
    FRLG_GBA_SCREEN_PIXELS = FRLG_GBA_SCREEN_WIDTH * FRLG_GBA_SCREEN_HEIGHT
};

typedef struct FrlgRgb8 {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} FrlgRgb8;

FrlgRgb8 frlg_gba_bgr555_to_rgb8(uint16_t color);
bool frlg_gba_mode3_render(const FrlgGbaMemory *memory,
                           const FrlgGbaDisplaySnapshot *display,
                           FrlgRgb8 *output,
                           size_t output_pixels);

#endif
