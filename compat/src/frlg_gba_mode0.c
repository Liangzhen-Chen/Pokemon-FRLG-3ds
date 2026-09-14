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
    int16_t pa, pb, pc, pd;
} ObjInfo;

static unsigned int obj_palette_index(const FrlgGbaMemory *memory, const ObjInfo *objects,
                                      const uint8_t *line_objects, unsigned int line_count,
                                      unsigned int x, unsigned int y,
                                      unsigned int *priority, bool *semi, bool window_only)
{
    for (unsigned int i = 0; i < line_count; i++)
    {
        const ObjInfo *object = objects + line_objects[i];
        const unsigned int attr0 = object->attr0;
        const unsigned int attr1 = object->attr1;
        const unsigned int attr2 = object->attr2;
        const unsigned int width = object->width;
        const unsigned int height = object->height;
        const bool affine = (attr0 & 0x100) != 0;
        const unsigned int draw_width = width * (affine && (attr0 & 0x200) ? 2 : 1);
        const unsigned int draw_height = height * (affine && (attr0 & 0x200) ? 2 : 1);
        int ox = attr1 & 511;
        int oy = attr0 & 255;
        if (!width || (((attr0 & 0x0c00) == 0x0800) != window_only))
            continue;
        if (ox >= 256)
            ox -= 512;
        if (oy >= 160)
            oy -= 256;
        if ((int)x < ox || (int)x >= ox + (int)draw_width ||
            (int)y < oy || (int)y >= oy + (int)draw_height)
            continue;
        int tx = (int)x - ox;
        int ty = (int)y - oy;
        if (affine)
        {
            const int dx = tx - (int)draw_width / 2;
            const int dy = ty - (int)draw_height / 2;
            tx = ((object->pa * dx + object->pb * dy) >> 8) + (int)width / 2;
            ty = ((object->pc * dx + object->pd * dy) >> 8) + (int)height / 2;
            if (tx < 0 || tx >= (int)width || ty < 0 || ty >= (int)height)
                continue;
        }
        else
        {
            if (attr1 & 0x1000)
                tx = (int)width - 1 - tx;
            if (attr1 & 0x2000)
                ty = (int)height - 1 - ty;
        }
        const unsigned int tile = (attr2 & 1023) + ((unsigned int)ty / 8) * (width / 8) + (unsigned int)tx / 8;
        const unsigned int packed = memory->vram[0x10000 + tile * 32 + ((unsigned int)ty & 7) * 4 + ((unsigned int)tx & 7) / 2];
        const unsigned int index = (packed >> (((unsigned int)tx & 1) * 4)) & 15;
        if (index)
        {
            *priority = (attr2 >> 10) & 3;
            *semi = (attr0 & 0x0c00) == 0x0400;
            return 256 + ((attr2 >> 12) & 15) * 16 + index;
        }
    }
    return 0;
}

static bool render_mode0(const FrlgGbaMemory *memory,
                         const FrlgGbaDisplaySnapshot *display,
                         FrlgRgb8 *output, size_t output_pixels,
                         const uint16_t *bldy_by_line)
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
    if (display->mode != 0)
        return false;

    const bool win0 = (display->control & 0x2000) != 0;
    const bool win1 = (display->control & 0x4000) != 0;
    const bool objwin = (display->control & 0x8000) != 0;
    unsigned int win_x1[2] = {0}, win_x2[2] = {0};
    unsigned int win_y1[2] = {0}, win_y2[2] = {0};
    unsigned int win_inside[2] = {0x3f, 0x3f};
    unsigned int win_outside = 0x3f, win_obj = 0x3f;
    if (win0 || win1 || objwin)
    {
        const unsigned int winin = read16(memory->io + 0x48);
        const unsigned int winout = read16(memory->io + 0x4a);
        for (unsigned int i = 0; i < 2; i++)
        {
            if (!(display->control & (0x2000u << i)))
                continue;
            const unsigned int horizontal = read16(memory->io + 0x40 + i * 2);
            const unsigned int vertical = read16(memory->io + 0x44 + i * 2);
            win_x1[i] = horizontal >> 8;
            win_x2[i] = horizontal & 255;
            win_y1[i] = vertical >> 8;
            win_y2[i] = vertical & 255;
            if (win_x1[i] > win_x2[i] || win_y1[i] > win_y2[i] ||
                win_x2[i] > FRLG_GBA_SCREEN_WIDTH || win_y2[i] > FRLG_GBA_SCREEN_HEIGHT)
                return false;
            win_inside[i] = (winin >> (i * 8)) & 0x3f;
        }
        win_outside = winout & 0x3f;
        win_obj = (winout >> 8) & 0x3f;
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
            if ((attr0 & (0x1000 | 0x2000)) ||
                (attr0 & 0x0c00) == 0x0c00 ||
                !obj_size(attr0 >> 14, attr1 >> 14, &width, &height) ||
                (attr2 & 1023) + width * height / 64 > 1024)
                return false;
            objects[i] = (ObjInfo){(uint16_t)attr0, (uint16_t)attr1, (uint16_t)attr2,
                                   (uint8_t)width, (uint8_t)height, 0, 0, 0, 0};
            if (attr0 & 0x100)
            {
                const unsigned int matrix = (attr1 >> 9) & 31;
                objects[i].pa = (int16_t)read16(memory->oam + (matrix * 4 + 0) * 8 + 6);
                objects[i].pb = (int16_t)read16(memory->oam + (matrix * 4 + 1) * 8 + 6);
                objects[i].pc = (int16_t)read16(memory->oam + (matrix * 4 + 2) * 8 + 6);
                objects[i].pd = (int16_t)read16(memory->oam + (matrix * 4 + 3) * 8 + 6);
            }
            if ((attr0 & 0x0c00) == 0x0400)
                has_semi_obj = true;
        }
    }
    /* The GF window keeps OBJ and effects enabled together. The mixed case
     * lacks a verified hardware rule, so reject it before touching output. */
    if (win1 && has_semi_obj &&
        (((win_inside[1] & 0x30) == 0x10) || ((win_outside & 0x30) == 0x10)))
        return false;
    const unsigned int bldcnt = read16(memory->io + 0x50);
    const unsigned int mode = (bldcnt >> 6) & 3;
    const unsigned int alpha = read16(memory->io + 0x52);
    const unsigned int eva = limited_coefficient(alpha & 31);
    const unsigned int evb = limited_coefficient((alpha >> 8) & 31);
    const unsigned int uniform_ey = limited_coefficient(read16(memory->io + 0x54) & 31);
    for (unsigned int y = 0; y < FRLG_GBA_SCREEN_HEIGHT; y++)
    {
    uint8_t line_objects[128];
    unsigned int line_count = 0;
    if (display->control & FRLG_GBA_DISPCNT_OBJ)
    {
        for (unsigned int i = 0; i < 128; i++)
        {
            const ObjInfo *object = objects + i;
            if (!object->width)
                continue;
            int oy = object->attr0 & 255;
            if (oy >= 160)
                oy -= 256;
            const bool affine = (object->attr0 & 0x100) != 0;
            const unsigned int draw_height = object->height * (affine && (object->attr0 & 0x200) ? 2 : 1);
            if ((int)y >= oy && (int)y < oy + (int)draw_height)
                line_objects[line_count++] = (uint8_t)i;
        }
    }
    for (unsigned int x = 0; x < FRLG_GBA_SCREEN_WIDTH; x++)
    {
        const unsigned int ey = bldy_by_line ? limited_coefficient(bldy_by_line[y] & 31) : uniform_ey;
        unsigned int window_mask = 0x3f;
        if (win0 || win1 || objwin)
        {
            window_mask = win_outside;
            if (objwin && line_count)
            {
                unsigned int ignored_priority = 0;
                bool ignored_semi = false;
                if (obj_palette_index(memory, objects, line_objects, line_count,
                                      x, y, &ignored_priority, &ignored_semi, true))
                    window_mask = win_obj;
            }
            if (win1 && x >= win_x1[1] && x < win_x2[1] &&
                y >= win_y1[1] && y < win_y2[1])
                window_mask = win_inside[1];
            if (win0 && x >= win_x1[0] && x < win_x2[0] &&
                y >= win_y1[0] && y < win_y2[0])
                window_mask = win_inside[0];
        }
        unsigned int top_layer = 5, second_layer = 5;
        unsigned int top_index = 0, second_index = 0;
        unsigned int obj_priority = 0;
        bool obj_semi = false, top_semi = false;
        const unsigned int obj_index = line_count && (window_mask & 0x10) ?
            obj_palette_index(memory, objects, line_objects, line_count,
                              x, y, &obj_priority, &obj_semi, false) : 0;
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
    }
    return true;
}

bool frlg_gba_mode0_render(const FrlgGbaMemory *memory,
                          const FrlgGbaDisplaySnapshot *display,
                          FrlgRgb8 *output, size_t output_pixels)
{
    return render_mode0(memory, display, output, output_pixels, NULL);
}

bool frlg_gba_mode0_render_with_bldy(const FrlgGbaMemory *memory,
                                    const FrlgGbaDisplaySnapshot *display,
                                    FrlgRgb8 *output, size_t output_pixels,
                                    const uint16_t bldy_by_line[FRLG_GBA_SCREEN_HEIGHT])
{
    return bldy_by_line && render_mode0(memory, display, output, output_pixels, bldy_by_line);
}
