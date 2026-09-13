#include "frlg_gba_mode3.h"

static uint8_t expand_5bit(uint16_t value)
{
    const uint8_t channel = (uint8_t)(value & 0x1f);
    return (uint8_t)((channel << 3) | (channel >> 2));
}

FrlgRgb8 frlg_gba_bgr555_to_rgb8(uint16_t color)
{
    const FrlgRgb8 result = {
        .red = expand_5bit(color),
        .green = expand_5bit(color >> 5),
        .blue = expand_5bit(color >> 10),
    };
    return result;
}

bool frlg_gba_mode3_render(const FrlgGbaMemory *memory,
                           const FrlgGbaDisplaySnapshot *display,
                           FrlgRgb8 *output,
                           size_t output_pixels)
{
    const uint8_t *vram;
    size_t pixel;

    if (memory == NULL || display == NULL || output == NULL ||
        output_pixels < FRLG_GBA_SCREEN_PIXELS)
        return false;

    if (display->forced_blank)
    {
        const FrlgRgb8 white = {255, 255, 255};
        for (pixel = 0; pixel < FRLG_GBA_SCREEN_PIXELS; pixel++)
            output[pixel] = white;
        return true;
    }

    if (display->mode != 3 ||
        (display->control & FRLG_GBA_DISPCNT_BG2) == 0 ||
        !frlg_gba_memory_map_const(memory, FRLG_GBA_VRAM_BASE,
            FRLG_GBA_SCREEN_PIXELS * 2, &vram))
        return false;

    for (pixel = 0; pixel < FRLG_GBA_SCREEN_PIXELS; pixel++)
    {
        const uint16_t color = (uint16_t)vram[pixel * 2] |
            (uint16_t)((uint16_t)vram[pixel * 2 + 1] << 8);
        output[pixel] = frlg_gba_bgr555_to_rgb8(color);
    }
    return true;
}
