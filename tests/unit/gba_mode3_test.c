#include <assert.h>
#include <stdint.h>

#include "frlg_gba_mode3.h"

static FrlgGbaMemory memory;
static FrlgRgb8 output[FRLG_GBA_SCREEN_PIXELS];

static void assert_color(FrlgRgb8 color, uint8_t red, uint8_t green, uint8_t blue)
{
    assert(color.red == red);
    assert(color.green == green);
    assert(color.blue == blue);
}

void run_gba_mode3_tests(void)
{
    FrlgGbaDisplaySnapshot display;

    assert_color(frlg_gba_bgr555_to_rgb8(UINT16_C(0x001f)), 255, 0, 0);
    assert_color(frlg_gba_bgr555_to_rgb8(UINT16_C(0x03e0)), 0, 255, 0);
    assert_color(frlg_gba_bgr555_to_rgb8(UINT16_C(0x7c00)), 0, 0, 255);
    assert_color(frlg_gba_bgr555_to_rgb8(UINT16_C(0x7fff)), 255, 255, 255);
    assert_color(frlg_gba_bgr555_to_rgb8(UINT16_C(0x4210)), 132, 132, 132);

    frlg_gba_memory_reset(&memory);
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_VRAM_BASE, UINT16_C(0x001f)));
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_VRAM_BASE + 2, UINT16_C(0x03e0)));
    assert(frlg_gba_memory_write16(&memory,
        FRLG_GBA_VRAM_BASE + (FRLG_GBA_SCREEN_PIXELS - 1) * 2,
        UINT16_C(0x7c00)));
    assert(frlg_gba_display_set_control(&memory, 3 | FRLG_GBA_DISPCNT_BG2));
    assert(frlg_gba_display_snapshot(&memory, &display));
    assert(frlg_gba_mode3_render(&memory, &display, output, FRLG_GBA_SCREEN_PIXELS));
    assert_color(output[0], 255, 0, 0);
    assert_color(output[1], 0, 255, 0);
    assert_color(output[FRLG_GBA_SCREEN_PIXELS - 1], 0, 0, 255);

    output[0] = (FrlgRgb8){1, 2, 3};
    assert(frlg_gba_display_set_control(&memory, 2 | FRLG_GBA_DISPCNT_BG2));
    assert(frlg_gba_display_snapshot(&memory, &display));
    assert(!frlg_gba_mode3_render(&memory, &display, output, FRLG_GBA_SCREEN_PIXELS));
    assert_color(output[0], 1, 2, 3);

    assert(frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_FORCED_BLANK));
    assert(frlg_gba_display_snapshot(&memory, &display));
    assert(frlg_gba_mode3_render(&memory, &display, output, FRLG_GBA_SCREEN_PIXELS));
    assert_color(output[0], 255, 255, 255);
    assert_color(output[FRLG_GBA_SCREEN_PIXELS - 1], 255, 255, 255);

    assert(!frlg_gba_mode3_render(NULL, &display, output, FRLG_GBA_SCREEN_PIXELS));
    assert(!frlg_gba_mode3_render(&memory, NULL, output, FRLG_GBA_SCREEN_PIXELS));
    assert(!frlg_gba_mode3_render(&memory, &display, NULL, FRLG_GBA_SCREEN_PIXELS));
    assert(!frlg_gba_mode3_render(&memory, &display, output, FRLG_GBA_SCREEN_PIXELS - 1));
}
