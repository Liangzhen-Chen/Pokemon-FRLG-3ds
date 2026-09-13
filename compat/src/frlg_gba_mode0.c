#include "frlg_gba_mode0.h"

static uint16_t read16(const uint8_t *bytes)
{
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static unsigned int background_palette_index(const FrlgGbaMemory *memory,
                                             const FrlgGbaDisplaySnapshot *display,
                                             unsigned int layer, unsigned int x, unsigned int y)
{
    const unsigned int control = display->background_control[layer];
    const unsigned int size = control >> 14;
    const unsigned int width = (size & 1) ? 512 : 256;
    const unsigned int height = (size & 2) ? 512 : 256;
    const unsigned int sx = (x + display->background_x[layer]) & (width - 1);
    const unsigned int sy = (y + display->background_y[layer]) & (height - 1);
    const unsigned int block = (sy / 256) * (width / 256) + sx / 256;
    const unsigned int map = ((control >> 8) & 31) * 2048;
    const unsigned int chars = ((control >> 2) & 3) * 16384;
    const unsigned int offset = block * 2048 + ((sy / 8 % 32) * 32 + sx / 8 % 32) * 2;
    const unsigned int entry = read16(memory->vram + map + offset);
    const unsigned int tx = (sx & 7) ^ ((entry & 0x400) ? 7 : 0);
    const unsigned int ty = (sy & 7) ^ ((entry & 0x800) ? 7 : 0);
    if (control & 0x80)
        return memory->vram[chars + (entry & 1023) * 64 + ty * 8 + tx];
    const unsigned int packed = memory->vram[chars + (entry & 1023) * 32 + ty * 4 + tx / 2];
    const unsigned int index = (packed >> ((tx & 1) * 4)) & 15;
    return index ? ((entry >> 12) * 16 + index) : 0;
}

static unsigned int limited_coefficient(unsigned int value)
{
    return value > 16 ? 16 : value;
}

static uint16_t effect_color(uint16_t first, uint16_t second, unsigned int mode,
                             unsigned int eva, unsigned int evb, unsigned int ey)
{
    uint16_t result = 0;
    for (unsigned int shift = 0; shift <= 10; shift += 5)
    {
        const unsigned int a = (first >> shift) & 31;
        const unsigned int b = (second >> shift) & 31;
        unsigned int channel = a;
        if (mode == 1)
        {
            channel = (a * eva + b * evb) / 16;
            if (channel > 31)
                channel = 31;
        }
        else if (mode == 2)
            channel = a + ((31 - a) * ey) / 16;
        else if (mode == 3)
            channel = a - (a * ey) / 16;
        result |= (uint16_t)(channel << shift);
    }
    return result;
}

bool frlg_gba_mode0_render(const FrlgGbaMemory *memory,
                          const FrlgGbaDisplaySnapshot *display,
                          FrlgRgb8 *output, size_t output_pixels)
{
    unsigned int bg, pixel;
    FrlgRgb8 palette[256];
    if (!memory || !display || !output || output_pixels < FRLG_GBA_SCREEN_PIXELS)
        return false;
    if (display->forced_blank)
    {
        for (pixel = 0; pixel < FRLG_GBA_SCREEN_PIXELS; pixel++)
            output[pixel] = (FrlgRgb8){255, 255, 255};
        return true;
    }
    /* OBJ and windows are separate future slices. */
    if (display->mode != 0 || (display->control & 0xf000))
        return false;

    /* Validate every enabled map before writing any output. BG data is limited
     * to the first 64 KiB; hardware overflow/mirroring is deliberately excluded. */
    for (bg = 0; bg < 4; bg++)
    {
        const unsigned int control = display->background_control[bg];
        const unsigned int size = control >> 14;
        const unsigned int blocks = (1u + (size & 1u)) * (1u + (size >> 1));
        const unsigned int map = ((control >> 8) & 31) * 2048;
        const unsigned int chars = ((control >> 2) & 3) * 16384;
        const unsigned int tile_bytes = (control & 0x80) ? 64 : 32;
        if (!(display->control & (0x100u << bg)))
            continue;
        if ((control & 0x40) || map + blocks * 2048 > 65536)
            return false;
        for (unsigned int entry = 0; entry < blocks * 1024; entry++)
            if (chars + (read16(memory->vram + map + entry * 2) & 1023) * tile_bytes + tile_bytes > 65536)
                return false;
    }
    for (pixel = 0; pixel < 256; pixel++)
        palette[pixel] = frlg_gba_bgr555_to_rgb8(read16(memory->palette + pixel * 2));

    const unsigned int bldcnt = read16(memory->io + 0x50);
    const unsigned int mode = (bldcnt >> 6) & 3;
    if (mode)
    {
        unsigned int order[4];
        unsigned int count = 0;
        const unsigned int alpha = read16(memory->io + 0x52);
        const unsigned int eva = limited_coefficient(alpha & 31);
        const unsigned int evb = limited_coefficient((alpha >> 8) & 31);
        const unsigned int ey = limited_coefficient(read16(memory->io + 0x54) & 31);
        const uint16_t backdrop = read16(memory->palette);
        for (unsigned int priority = 0; priority < 4; priority++)
            for (unsigned int layer = 0; layer < 4; layer++)
                if ((display->control & (0x100u << layer)) &&
                    (display->background_control[layer] & 3) == priority)
                    order[count++] = layer;
        for (unsigned int y = 0; y < FRLG_GBA_SCREEN_HEIGHT; y++)
            for (unsigned int x = 0; x < FRLG_GBA_SCREEN_WIDTH; x++)
            {
                unsigned int top_layer = 5;
                unsigned int second_layer = 5;
                unsigned int top_index = 0;
                unsigned int second_index = 0;
                for (unsigned int i = 0; i < count; i++)
                {
                    const unsigned int index = background_palette_index(memory, display, order[i], x, y);
                    if (!index)
                        continue;
                    if (top_layer == 5)
                    {
                        top_layer = order[i];
                        top_index = index;
                    }
                    else
                    {
                        second_layer = order[i];
                        second_index = index;
                        break;
                    }
                }
                const uint16_t top = top_layer == 5 ? backdrop :
                    read16(memory->palette + top_index * 2);
                const uint16_t second = second_layer == 5 ? backdrop :
                    read16(memory->palette + second_index * 2);
                uint16_t result = top;
                if (bldcnt & (1u << top_layer))
                {
                    if (mode == 1 && top_layer != 5 && (bldcnt & (1u << (second_layer + 8))))
                        result = effect_color(top, second, mode, eva, evb, ey);
                    else if (mode == 2 || mode == 3)
                        result = effect_color(top, 0, mode, eva, evb, ey);
                }
                output[y * FRLG_GBA_SCREEN_WIDTH + x] = frlg_gba_bgr555_to_rgb8(result);
            }
        return true;
    }
    for (pixel = 0; pixel < FRLG_GBA_SCREEN_PIXELS; pixel++)
        output[pixel] = palette[0];

    /* Back to front: lower priority value wins; ties favor the lower BG index. */
    for (int priority = 3; priority >= 0; priority--)
    for (int layer = 3; layer >= 0; layer--)
    {
        const unsigned int control = display->background_control[layer];
        const unsigned int size = control >> 14;
        const unsigned int width = (size & 1) ? 512 : 256;
        const unsigned int height = (size & 2) ? 512 : 256;
        const unsigned int map = ((control >> 8) & 31) * 2048;
        const unsigned int chars = ((control >> 2) & 3) * 16384;
        if (!(display->control & (0x100u << layer)) || (control & 3) != (unsigned)priority)
            continue;
        for (unsigned int y = 0; y < FRLG_GBA_SCREEN_HEIGHT; y++)
        for (unsigned int x = 0; x < FRLG_GBA_SCREEN_WIDTH; x++)
        {
            const unsigned int sx = (x + display->background_x[layer]) & (width - 1);
            const unsigned int sy = (y + display->background_y[layer]) & (height - 1);
            const unsigned int block = (sy / 256) * (width / 256) + sx / 256;
            const unsigned int offset = block * 2048 + ((sy / 8 % 32) * 32 + sx / 8 % 32) * 2;
            const unsigned int entry = read16(memory->vram + map + offset);
            const unsigned int tx = (sx & 7) ^ ((entry & 0x400) ? 7 : 0);
            const unsigned int ty = (sy & 7) ^ ((entry & 0x800) ? 7 : 0);
            unsigned int index;
            unsigned int bank = 0;
            if (control & 0x80) {
                index = memory->vram[chars + (entry & 1023) * 64 + ty * 8 + tx];
            } else {
                const unsigned int packed = memory->vram[chars + (entry & 1023) * 32 + ty * 4 + tx / 2];
                index = (packed >> ((tx & 1) * 4)) & 15;
                bank = (entry >> 12) * 16;
            }
            if (index)
                output[y * FRLG_GBA_SCREEN_WIDTH + x] = palette[bank + index];
        }
    }
    return true;
}
