#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "frlg_gba_mode0.h"

static FrlgGbaMemory memory;
static FrlgRgb8 pixels[FRLG_GBA_SCREEN_PIXELS + 1];
static FrlgGbaDisplaySnapshot display;

static void render(void)
{
    assert(frlg_gba_display_snapshot(&memory, &display));
    assert(frlg_gba_mode0_render(&memory, &display, pixels, FRLG_GBA_SCREEN_PIXELS));
}

static void entry(unsigned int block, unsigned int index, uint16_t value)
{
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_VRAM_BASE + block * 2048 + index * 2, value));
}

static void color(unsigned int index, uint16_t value)
{
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_PALETTE_BASE + index * 2, value));
}

static void expect(unsigned int x, unsigned int y, int red, int green, int blue)
{
    FrlgRgb8 p = pixels[y * 240 + x];
    if (p.red != red || p.green != green || p.blue != blue)
        fprintf(stderr, "mode0 pixel (%u,%u): got %u,%u,%u expected %d,%d,%d\n",
                x, y, p.red, p.green, p.blue, red, green, blue);
    assert(p.red == red && p.green == green && p.blue == blue);
}

static void rejected(void)
{
    memset(pixels, 0x5a, sizeof(pixels));
    assert(frlg_gba_display_snapshot(&memory, &display));
    assert(!frlg_gba_mode0_render(&memory, &display, pixels, FRLG_GBA_SCREEN_PIXELS));
    for (unsigned int i = 0; i <= FRLG_GBA_SCREEN_PIXELS; i++)
        assert(pixels[i].red == 0x5a && pixels[i].green == 0x5a && pixels[i].blue == 0x5a);
}

static void obj(unsigned int number, uint16_t attr0, uint16_t attr1, uint16_t attr2)
{
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_OAM_BASE + number * 8, attr0));
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_OAM_BASE + number * 8 + 2, attr1));
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_OAM_BASE + number * 8 + 4, attr2));
}

static void run_obj_tests(void)
{
    frlg_gba_memory_reset(&memory);
    for (unsigned int i = 0; i < 128; i++)
        obj(i, 0x0200, 0, 0);
    color(0, 0x7c00); color(1, 0x03e0);
    color(256 + 1, 0x001f); color(256 + 2, 0x7fff);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG3 | FRLG_GBA_DISPCNT_OBJ | 0x40);
    frlg_gba_display_set_background_control(&memory, 3, (16 << 8) | 2);
    entry(16, 1 + 5 * 32, 1);
    memset(memory.vram + 32, 0x11, 32);
    memory.vram[0x10000 + 32] = 0x01;
    memset(memory.vram + 0x10000 + 33, 0x11, 31);
    obj(0, 40, 8, 1);
    render();
    expect(8, 40, 255, 0, 0);
    expect(9, 40, 0, 255, 0);
    expect(7, 40, 0, 0, 255);

    /* OAM index wins between OBJs even when their BG-relative priorities differ. */
    memset(memory.vram + 0x10000 + 64, 0x22, 32);
    obj(1, 40, 8, 2);
    render(); expect(8, 40, 255, 0, 0);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_OBJ | 0x40);
    obj(0, 40, 8, 1 | 0x0c00);
    obj(1, 40, 8, 2);
    render(); expect(8, 40, 255, 0, 0);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG3 | FRLG_GBA_DISPCNT_OBJ | 0x40);
    render(); expect(8, 40, 0, 255, 0);
    obj(1, 0x0200, 0, 0);
    obj(0, 40, 8, 1 | 0x0800);
    render(); expect(8, 40, 255, 0, 0);
    obj(0, 40, 8, 1);
    render(); expect(8, 40, 255, 0, 0);

    /* Semi OBJ forces alpha; a covered second OBJ is not a blend target. */
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x52, 0x0808);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x0840);
    obj(0, 40 | 0x0400, 8, 1);
    obj(1, 0x0200, 0, 0);
    render(); expect(8, 40, 123, 123, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x0800);
    render(); expect(8, 40, 123, 123, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x1040);
    obj(1, 40, 8, 2);
    render(); expect(8, 40, 255, 0, 0);

    /* A 16x16 1D sprite advances two tiles at the next tile row. */
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_OBJ | 0x40);
    obj(1, 0x0200, 0, 0);
    memset(memory.vram + 0x10000 + 4 * 32, 0x11, 32);
    memset(memory.vram + 0x10000 + 5 * 32, 0x22, 32);
    memset(memory.vram + 0x10000 + 6 * 32, 0x22, 32);
    memset(memory.vram + 0x10000 + 7 * 32, 0x11, 32);
    obj(0, 60, 20 | 0x4000, 4);
    render();
    expect(20, 60, 255, 0, 0); expect(28, 60, 255, 255, 255);
    expect(20, 68, 255, 255, 255); expect(28, 68, 255, 0, 0);
    obj(0, 60, 20 | 0x5000, 4);
    render(); expect(20, 60, 255, 255, 255);
    obj(0, 60, 20 | 0x6000, 4);
    render(); expect(20, 60, 255, 255, 255);

    /* The logo's 32x64 shape and right/bottom screen clipping. */
    memset(memory.vram + 0x10000 + 8 * 32, 0x11, 32);
    memset(memory.vram + 0x10000 + 39 * 32, 0x22, 32);
    obj(0, 80 | 0x8000, 50 | 0xc000, 8);
    render(); expect(50, 80, 255, 0, 0); expect(81, 143, 255, 255, 255);
    obj(0, 155, 236 | 0x4000, 4);
    render(); expect(236, 155, 255, 0, 0); expect(239, 159, 255, 0, 0);
    expect(235, 155, 0, 0, 255);

    /* Unsupported enabled OAM never changes any output pixel. */
    obj(0, 0xc000, 0, 4); rejected();
    obj(0, 0x0100, 0, 4); rejected();
    obj(0, 0x1000, 0, 4); rejected();
    obj(0, 0x2000, 0, 4); rejected();
    obj(0, 0x0800, 0, 4); rejected();
    obj(0, 0x0c00, 0, 4); rejected();
    obj(0, 60, 20 | 0x4000, 1023); rejected();
    obj(0, 0xe200, 0, 4);
    render(); expect(20, 60, 0, 0, 255);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_OBJ);
    rejected();
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_OBJ | 0x40);
    memset(memory.oam, 0, sizeof(memory.oam));
    render(); expect(8, 40, 0, 0, 255);
}

static void run_win1_tests(void)
{
    frlg_gba_memory_reset(&memory);
    for (unsigned int i = 0; i < 128; i++)
        obj(i, 0x0200, 0, 0);
    color(0, 0x7c00); color(1, 0x03e0); color(2, 0x001f);
    color(256 + 1, 0x001f);
    memset(memory.vram, 0x11, 32);
    memset(memory.vram + 0x10000 + 32, 0x11, 32);
    memory.vram[0x10000 + 32] = 0x01;
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG3 | FRLG_GBA_DISPCNT_OBJ | 0x4040);
    frlg_gba_display_set_background_control(&memory, 3, (16 << 8) | 2);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x42, 0x00f0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x46, 0x2080);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x3f00);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x4a, 0);
    obj(0, 40, 8, 1);
    obj(1, 128, 8, 1);
    render();
    expect(8, 31, 0, 0, 255); expect(8, 32, 0, 255, 0);
    expect(8, 127, 0, 255, 0); expect(8, 128, 0, 0, 255);
    expect(8, 40, 255, 0, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x42, 0x08e8);
    render();
    expect(7, 32, 0, 0, 255); expect(8, 32, 0, 255, 0);
    expect(231, 32, 0, 255, 0); expect(232, 32, 0, 0, 255);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x42, 0x00f0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x4a, 0x0008);
    render(); expect(8, 31, 0, 255, 0); expect(8, 40, 0, 0, 255);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x3f00);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x4a, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x46, 0x4858);
    render();
    expect(8, 71, 0, 0, 255); expect(8, 72, 0, 255, 0);
    expect(8, 87, 0, 255, 0); expect(8, 88, 0, 0, 255);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x46, 0);
    render(); expect(8, 40, 0, 0, 255);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x46, 0x2080);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x42, 0);
    render(); expect(8, 40, 0, 0, 255);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x42, 0x00f0);

    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x0800);
    render(); expect(8, 40, 0, 255, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x1000);
    render(); expect(8, 40, 255, 0, 0); expect(9, 40, 0, 0, 255);

    obj(0, 0x0200, 0, 0); obj(1, 0x0200, 0, 0);
    memset(memory.vram + 16384, 0x22, 32);
    frlg_gba_display_set_background_control(&memory, 2, (20 << 8) | 4);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG2 | FRLG_GBA_DISPCNT_BG3 | 0x4000);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x52, 0x0808);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x3f44);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x3f00);
    render(); expect(8, 40, 123, 123, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x1f00);
    render(); expect(8, 40, 255, 0, 0);

    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG3 | FRLG_GBA_DISPCNT_OBJ | 0x4040);
    obj(0, 40 | 0x0400, 8, 1);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x3f50);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x3f00);
    render(); expect(8, 40, 123, 123, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x48, 0x1f00);
    rejected();
    obj(0, 0x0200, 0, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x46, 0x8020);
    rejected();
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x46, 0x2080);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x42, 0xf008);
    rejected();
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x42, 0x00f0);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG3 | 0x2000);
    rejected();
}

void run_gba_mode0_tests(void)
{
    frlg_gba_memory_reset(&memory);
    color(0, 0x7c00); color(1, 0x001f); color(2, 0x03e0); color(17, 0x7fff);
    memset(memory.vram + 32, 0x11, 32);
    memory.vram[32] = 0x21;
    memory.vram[63] = 0x02;
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0);
    frlg_gba_display_set_background_control(&memory, 0, 16 << 8);
    entry(16, 0, 1);
    pixels[FRLG_GBA_SCREEN_PIXELS] = (FrlgRgb8){9, 8, 7};
    render();
    expect(0, 0, 255, 0, 0); expect(1, 0, 0, 255, 0);
    expect(7, 7, 0, 0, 255); expect(239, 159, 0, 0, 255);
    assert(pixels[FRLG_GBA_SCREEN_PIXELS].red == 9);
    entry(16, 0, 1 | 0x400); render(); expect(6, 0, 0, 255, 0);
    entry(16, 0, 1 | 0x800); render(); expect(6, 0, 0, 255, 0);
    entry(16, 0, 1 | 0xc00); render(); expect(0, 0, 0, 0, 255); expect(1, 0, 0, 255, 0);
    entry(16, 0, 1 | 0x1000); render(); expect(0, 0, 255, 255, 255);

    /* Every screen size: block ordering at x/y=256, plus wrapping. */
    for (unsigned int size = 0; size < 4; size++)
    {
        unsigned int width = (size & 1) ? 512 : 256;
        unsigned int height = (size & 2) ? 512 : 256;
        memset(memory.vram + 32768, 0, 8192);
        frlg_gba_display_set_background_control(&memory, 0, (uint16_t)((16 << 8) | (size << 14)));
        unsigned int sx = width - 256, sy = height - 256;
        unsigned int block = (sy / 256) * (width / 256) + sx / 256;
        entry(16 + block, 0, 1);
        frlg_gba_memory_write16(&memory, FRLG_GBA_REG_BG0HOFS, (uint16_t)sx);
        frlg_gba_memory_write16(&memory, FRLG_GBA_REG_BG0VOFS, (uint16_t)sy);
        render(); expect(0, 0, 255, 0, 0);
        entry(16, 0, 1);
        frlg_gba_memory_write16(&memory, FRLG_GBA_REG_BG0HOFS, (uint16_t)(width - 1));
        frlg_gba_memory_write16(&memory, FRLG_GBA_REG_BG0VOFS, (uint16_t)(height - 1));
        render(); expect(1, 1, 255, 0, 0); expect(2, 1, 0, 255, 0);
    }
    /* 9-bit offset masking, character base and priority ties. */
    frlg_gba_memory_write16(&memory, FRLG_GBA_REG_BG0HOFS, 0xfe00);
    frlg_gba_memory_write16(&memory, FRLG_GBA_REG_BG0VOFS, 0xfe00);
    frlg_gba_display_set_background_control(&memory, 0, 16 << 8);
    frlg_gba_display_set_background_control(&memory, 1, (20 << 8) | 4);
    memset(memory.vram + 16384 + 32, 0x22, 32);
    entry(20, 0, 1);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0 | FRLG_GBA_DISPCNT_BG1);
    render(); assert(display.background_x[0] == 0 && display.background_y[0] == 0);
    expect(0, 0, 255, 0, 0); expect(7, 7, 0, 255, 0);
    frlg_gba_display_set_background_control(&memory, 0, (16 << 8) | 1);
    render(); expect(0, 0, 0, 255, 0);
    frlg_gba_display_set_control(&memory, 0); render(); expect(0, 0, 0, 0, 255);
    /* BG2 and BG3 have the same regular-background semantics in Mode 0. */
    for (unsigned int bg = 2; bg < 4; bg++)
    {
        frlg_gba_display_set_background_control(&memory, bg, (20 << 8) | 4);
        frlg_gba_display_set_control(&memory, (uint16_t)(0x100 << bg));
        render(); expect(0, 0, 0, 255, 0);
    }
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0);
    frlg_gba_display_set_background_control(&memory, 0, (16 << 8) | 0x40); rejected();
    frlg_gba_display_set_background_control(&memory, 0, (31 << 8) | 0xc000); rejected();
    frlg_gba_display_set_background_control(&memory, 0, (16 << 8) | 12);
    entry(16, 0, 512); rejected();
    entry(16, 0, 1);
    frlg_gba_display_set_control(&memory, 3); rejected();
    frlg_gba_display_set_control(&memory, 0x2000); rejected();
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_OBJ); rejected();
    frlg_gba_display_set_control(&memory, 0);
    /* Last legal tile in character block 3 reaches exactly the BG VRAM limit. */
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0);
    frlg_gba_display_set_background_control(&memory, 0, (16 << 8) | 12);
    entry(16, 0, 511);
    memset(memory.vram + 65504, 0x11, 32);
    render(); expect(0, 0, 255, 0, 0);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_FORCED_BLANK | 3);
    render(); expect(0, 0, 255, 255, 255); expect(239, 159, 255, 255, 255);
    assert(!frlg_gba_mode0_render(NULL, &display, pixels, FRLG_GBA_SCREEN_PIXELS));
    assert(!frlg_gba_mode0_render(&memory, NULL, pixels, FRLG_GBA_SCREEN_PIXELS));
    assert(!frlg_gba_mode0_render(&memory, &display, NULL, FRLG_GBA_SCREEN_PIXELS));
    assert(!frlg_gba_mode0_render(&memory, &display, pixels, FRLG_GBA_SCREEN_PIXELS - 1));

    /* 8bpp: one byte per pixel, 64 bytes per tile, full shared palette.
     * Screen-entry palette-bank bits are ignored, unlike 4bpp. */
    frlg_gba_memory_reset(&memory);
    color(0, 0x7c00); color(1, 0x001f); color(17, 0x03e0); color(255, 0x7fff);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0);
    frlg_gba_display_set_background_control(&memory, 0, (16 << 8) | 0x80);
    memory.vram[64] = 17;
    memory.vram[65] = 255;
    memory.vram[71] = 1;
    memory.vram[120] = 255;
    entry(16, 0, 1 | 0xf000);
    render(); expect(0,0,0,255,0); expect(1,0,255,255,255);
    expect(2,0,0,0,255); expect(7,0,255,0,0);
    entry(16,0,1 | 0xf400); render(); expect(0,0,255,0,0); expect(7,0,0,255,0);
    entry(16,0,1 | 0xf800); render(); expect(0,0,255,255,255);
    entry(16,0,1 | 0xfc00); render(); expect(7,0,255,255,255);

    /* Transparent 8bpp pixels reveal an independently decoded 4bpp BG1. */
    entry(16,0,1);
    memset(memory.vram + 16384 + 32, 0x11,32);
    frlg_gba_display_set_background_control(&memory,1,(20 << 8) | 4 | 1);
    entry(20,0,1 | 0x1000);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0 | FRLG_GBA_DISPCNT_BG1);
    render(); expect(1,0,255,255,255); expect(2,0,0,255,0);
    frlg_gba_display_set_background_control(&memory,0,(16 << 8) | 0x80 | 2);
    render(); expect(1,0,0,255,0);

    /* Last valid 8bpp tile in CBB3, and first overflowing tile. */
    frlg_gba_display_set_control(&memory,FRLG_GBA_DISPCNT_BG0);
    frlg_gba_display_set_background_control(&memory,0,(16 << 8) | 0x80 | 12);
    memset(memory.vram + 65472,255,64);
    entry(16,0,255); render(); expect(0,0,255,255,255); expect(7,7,255,255,255);
    entry(16,0,256); rejected();

    /* 8bpp uses the same screenblock order and wrap for all four map sizes. */
    for (unsigned size = 0; size < 4; size++) {
        unsigned width = size & 1 ? 512 : 256;
        unsigned height = size & 2 ? 512 : 256;
        unsigned block = ((height - 256) / 256) * (width / 256) + (width - 256) / 256;
        memset(memory.vram + 32768,0,8192);
        frlg_gba_display_set_background_control(&memory,0,(uint16_t)((16 << 8) | 0x80 | (size << 14)));
        entry(16 + block,0,1);
        frlg_gba_memory_write16(&memory,FRLG_GBA_REG_BG0HOFS,(uint16_t)(width - 256));
        frlg_gba_memory_write16(&memory,FRLG_GBA_REG_BG0VOFS,(uint16_t)(height - 256));
        render(); expect(0,0,0,255,0);
        entry(16,0,1);
        frlg_gba_memory_write16(&memory,FRLG_GBA_REG_BG0HOFS,(uint16_t)(width - 1));
        frlg_gba_memory_write16(&memory,FRLG_GBA_REG_BG0VOFS,(uint16_t)(height - 1));
        render(); expect(1,1,0,255,0); expect(2,1,255,255,255);
    }

    /* Color effects use the frontmost and immediately following visible pixel. */
    frlg_gba_memory_reset(&memory);
    color(0, 0x7c00); color(1, 0x001f); color(17, 0x03e0);
    memset(memory.vram + 32, 0x11, 32);
    memset(memory.vram + 16384 + 32, 0x11, 32);
    entry(16, 0, 1); entry(20, 0, 1 | 0x1000);
    frlg_gba_display_set_background_control(&memory, 0, 16 << 8);
    frlg_gba_display_set_background_control(&memory, 1, (20 << 8) | 4 | 1);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0 | FRLG_GBA_DISPCNT_BG1);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x52, 0x0808);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x0241);
    render(); expect(0, 0, 123, 123, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x52, 0x1f1f);
    render(); expect(0, 0, 255, 255, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x52, 0x0808);
    memory.vram[32] = 0;
    render(); expect(0, 0, 0, 255, 0);
    memory.vram[32] = 0x11;
    frlg_gba_display_set_background_control(&memory, 0, (16 << 8) | 0x80);
    memory.vram[64] = 1;
    render(); expect(0, 0, 123, 123, 0);
    frlg_gba_display_set_background_control(&memory, 0, 16 << 8);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x0441);
    render(); expect(0, 0, 255, 0, 0);
    color(33, 0x7c00);
    memset(memory.vram + 49152 + 32, 0x11, 32);
    entry(24, 0, 1 | 0x2000);
    frlg_gba_display_set_background_control(&memory, 2, (24 << 8) | 12 | 2);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0 | FRLG_GBA_DISPCNT_BG1 | FRLG_GBA_DISPCNT_BG2);
    render(); expect(0, 0, 255, 0, 0);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0 | FRLG_GBA_DISPCNT_BG1);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x0141);
    render(); expect(0, 0, 255, 0, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x2041);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_BG0);
    render(); expect(0, 0, 123, 0, 123);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x54, 8);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x0081);
    render(); expect(0, 0, 255, 123, 123);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x00c1);
    render(); expect(0, 0, 132, 0, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x54, 31);
    render(); expect(0, 0, 0, 0, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x0080);
    render(); expect(0, 0, 255, 0, 0);
    frlg_gba_display_set_control(&memory, 0);
    frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + 0x50, 0x00a0);
    render(); expect(0, 0, 255, 255, 255);
    frlg_gba_display_set_control(&memory, FRLG_GBA_DISPCNT_FORCED_BLANK | 3);
    render(); expect(0, 0, 255, 255, 255);
    run_obj_tests();
    run_win1_tests();
}
