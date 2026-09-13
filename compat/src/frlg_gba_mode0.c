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

static bool obj_size(unsigned int shape, unsigned int size, unsigned int *width, unsigned int *height)
{
    static const uint8_t widths[3][4] = {
        {8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}
    };
    static const uint8_t heights[3][4] = {
        {8, 16, 32, 64}, {8, 8, 16, 32}, {16, 32, 32, 64}
    };
    if (shape >= 3)
        return false;
    *width = widths[shape][size];
    *height = heights[shape][size];
    return true;
}

typedef struct {
    uint16_t attr0, attr1, attr2;
    uint8_t width, height;
} ObjInfo;

static unsigned int obj_palette_index(const FrlgGbaMemory *memory, const ObjInfo *objects,
                                      unsigned int x, unsigned int y,
                                      unsigned int *priority, bool *semi)
{
    for (unsigned int i = 0; i < 128; i++)
    {
        const ObjInfo *object = objects + i;
        const unsigned int attr0 = object->attr0;
        const unsigned int attr1 = object->attr1;
        const unsigned int attr2 = object->attr2;
        const unsigned int width = object->width;
        const unsigned int height = object->height;
        int ox = attr1 & 511;
        int oy = attr0 & 255;
        if (!width)
            continue;
        if (ox >= 256)
            ox -= 512;
        if (oy >= 160)
            oy -= 256;
        if ((int)x < ox || (int)x >= ox + (int)width ||
            (int)y < oy || (int)y >= oy + (int)height)
            continue;
        unsigned int tx = (unsigned int)((int)x - ox);
        unsigned int ty = (unsigned int)((int)y - oy);
        if (attr1 & 0x1000)
            tx = width - 1 - tx;
        if (attr1 & 0x2000)
            ty = height - 1 - ty;
        const unsigned int tile = (attr2 & 1023) + (ty / 8) * (width / 8) + tx / 8;
        const unsigned int packed = memory->vram[0x10000 + tile * 32 + (ty & 7) * 4 + (tx & 7) / 2];
        const unsigned int index = (packed >> ((tx & 1) * 4)) & 15;
        if (index)
        {
            *priority = (attr2 >> 10) & 3;
            *semi = (attr0 & 0x0c00) == 0x0400;
            return 256 + ((attr2 >> 12) & 15) * 16 + index;
        }
    }
    return 0;
}

bool frlg_gba_mode0_render(const FrlgGbaMemory *memory,
                          const FrlgGbaDisplaySnapshot *display,
                          FrlgRgb8 *output, size_t output_pixels)
{
    unsigned int bg, pixel;
    if (!memory || !display || !output || output_pixels < FRLG_GBA_SCREEN_PIXELS)
        return false;
    if (display->forced_blank)
    {
        for (pixel = 0; pixel < FRLG_GBA_SCREEN_PIXELS; pixel++)
            output[pixel] = (FrlgRgb8){255, 255, 255};
        return true;
    }
    if (display->mode != 0 || (display->control & 0xa000))
        return false;

    const bool win1 = (display->control & 0x4000) != 0;
    unsigned int win_x1 = 0, win_x2 = 0, win_y1 = 0, win_y2 = 0;
    unsigned int win_inside = 0x3f, win_outside = 0x3f;
    if (win1)
    {
        const unsigned int horizontal = read16(memory->io + 0x42);
        const unsigned int vertical = read16(memory->io + 0x46);
        win_x1 = horizontal >> 8;
        win_x2 = horizontal & 255;
        win_y1 = vertical >> 8;
        win_y2 = vertical & 255;
        if (win_x1 > win_x2 || win_y1 > win_y2 || win_x2 > FRLG_GBA_SCREEN_WIDTH ||
            win_y2 > FRLG_GBA_SCREEN_HEIGHT)
            return false;
        win_inside = (read16(memory->io + 0x48) >> 8) & 0x3f;
        win_outside = read16(memory->io + 0x4a) & 0x3f;
    }

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
    ObjInfo objects[128] = {0};
    bool has_semi_obj = false;
    if (display->control & FRLG_GBA_DISPCNT_OBJ)
    {
        if (!(display->control & 0x40))
            return false;
        for (unsigned int i = 0; i < 128; i++)
        {
            const uint8_t *entry = memory->oam + i * 8;
            const unsigned int attr0 = read16(entry);
            const unsigned int attr1 = read16(entry + 2);
            const unsigned int attr2 = read16(entry + 4);
            unsigned int width, height;
            if (!(attr0 & 0x100) && (attr0 & 0x200))
                continue;
            if ((attr0 & (0x100 | 0x1000 | 0x2000)) ||
                (attr0 & 0x0c00) >= 0x0800 ||
                !obj_size(attr0 >> 14, attr1 >> 14, &width, &height) ||
                (attr2 & 1023) + width * height / 64 > 1024)
                return false;
            objects[i] = (ObjInfo){(uint16_t)attr0, (uint16_t)attr1, (uint16_t)attr2,
                                   (uint8_t)width, (uint8_t)height};
            if ((attr0 & 0x0c00) == 0x0400)
                has_semi_obj = true;
        }
    }
    /* The GF window keeps OBJ and effects enabled together. The mixed case
     * lacks a verified hardware rule, so reject it before touching output. */
    if (win1 && has_semi_obj &&
        (((win_inside & 0x30) == 0x10) || ((win_outside & 0x30) == 0x10)))
        return false;
    const unsigned int bldcnt = read16(memory->io + 0x50);
    const unsigned int mode = (bldcnt >> 6) & 3;
    const unsigned int alpha = read16(memory->io + 0x52);
    const unsigned int eva = limited_coefficient(alpha & 31);
    const unsigned int evb = limited_coefficient((alpha >> 8) & 31);
    const unsigned int ey = limited_coefficient(read16(memory->io + 0x54) & 31);
    for (unsigned int y = 0; y < FRLG_GBA_SCREEN_HEIGHT; y++)
    for (unsigned int x = 0; x < FRLG_GBA_SCREEN_WIDTH; x++)
    {
        const unsigned int window_mask = !win1 ? 0x3f :
            (x >= win_x1 && x < win_x2 && y >= win_y1 && y < win_y2 ? win_inside : win_outside);
        unsigned int top_layer = 5, second_layer = 5;
        unsigned int top_index = 0, second_index = 0;
        unsigned int obj_priority = 0;
        bool obj_semi = false, top_semi = false;
        const unsigned int obj_index = (display->control & FRLG_GBA_DISPCNT_OBJ) &&
            (window_mask & 0x10) ?
            obj_palette_index(memory, objects, x, y, &obj_priority, &obj_semi) : 0;
        for (unsigned int priority = 0; priority < 4; priority++)
        {
            if (obj_index && obj_priority == priority)
            {
                if (top_layer == 5)
                {
                    top_layer = 4;
                    top_index = obj_index;
                    top_semi = obj_semi;
                }
                else if (second_layer == 5)
                {
                    second_layer = 4;
                    second_index = obj_index;
                }
            }
            for (unsigned int layer = 0; layer < 4; layer++)
            {
                if (!(window_mask & (1u << layer)) ||
                    !(display->control & (0x100u << layer)) ||
                    (display->background_control[layer] & 3) != priority)
                    continue;
                const unsigned int index = background_palette_index(memory, display, layer, x, y);
                if (!index)
                    continue;
                if (top_layer == 5)
                {
                    top_layer = layer;
                    top_index = index;
                }
                else if (second_layer == 5)
                {
                    second_layer = layer;
                    second_index = index;
                }
            }
            if (second_layer != 5)
                break;
        }
        const uint16_t top = read16(memory->palette + top_index * 2);
        const uint16_t second = read16(memory->palette + second_index * 2);
        uint16_t result = top;
        if ((window_mask & 0x20) && top_semi && (bldcnt & (1u << (second_layer + 8))))
            result = effect_color(top, second, 1, eva, evb, ey);
        else if ((window_mask & 0x20) && (bldcnt & (1u << top_layer)))
        {
            if (mode == 1 && top_layer != 5 && (bldcnt & (1u << (second_layer + 8))))
                result = effect_color(top, second, 1, eva, evb, ey);
            else if (mode == 2 || mode == 3)
                result = effect_color(top, 0, mode, eva, evb, ey);
        }
        output[y * FRLG_GBA_SCREEN_WIDTH + x] = frlg_gba_bgr555_to_rgb8(result);
    }
    return true;
}
