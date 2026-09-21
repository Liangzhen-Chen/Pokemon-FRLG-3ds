#include "frlg_gba_mode0.h"

static uint16_t read16(const uint8_t *bytes)
{
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

typedef struct {
    unsigned int control, x_scroll, y_scroll, x_mask, y_mask, map, chars, row_stride;
    unsigned int row_map, tile_row, tile_y;
} BgInfo;

#ifndef FRLG_GBA_MODE0_REFERENCE_CANDIDATE
static unsigned int background_palette_index(const FrlgGbaMemory *memory,
                                             const BgInfo *background, unsigned int x)
{
    const unsigned int sx = (x + background->x_scroll) & background->x_mask;
    const unsigned int offset = background->row_map + (sx / 256) * 2048 +
                                background->tile_row + (sx / 8 % 32) * 2;
    const unsigned int entry = read16(memory->vram + offset);
    const unsigned int tx = (sx & 7) ^ ((entry & 0x400) ? 7 : 0);
    const unsigned int ty = background->tile_y ^ ((entry & 0x800) ? 7 : 0);
    if (background->control & 0x80)
        return memory->vram[background->chars + (entry & 1023) * 64 + ty * 8 + tx];
    const unsigned int packed = memory->vram[background->chars + (entry & 1023) * 32 + ty * 4 + tx / 2];
    const unsigned int index = (packed >> ((tx & 1) * 4)) & 15;
    return index ? ((entry >> 12) * 16 + index) : 0;
}
#endif

static unsigned int limited_coefficient(unsigned int value)
{
    return value > 16 ? 16 : value;
}

#ifndef FRLG_GBA_MODE0_REFERENCE_CANDIDATE
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
#endif

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
    int16_t ox, oy;
    uint8_t draw_width, draw_height;
} ObjInfo;

#ifndef FRLG_GBA_MODE0_REFERENCE_CANDIDATE
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
        const unsigned int draw_width = object->draw_width;
        const unsigned int draw_height = object->draw_height;
        const int ox = object->ox;
        const int oy = object->oy;
        if (((attr0 & 0x0c00) == 0x0800) != window_only)
            continue;
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
#endif

typedef struct {
    bool win0, win1, objwin;
    unsigned int win_x1[2], win_x2[2], win_y1[2], win_y2[2];
    unsigned int win_inside[2], win_outside, win_obj;
    BgInfo backgrounds[4];
    uint8_t priority_layers[4][4];
    unsigned int priority_counts[4];
    ObjInfo objects[128];
    unsigned int bldcnt, blend_mode, eva, evb, uniform_ey;
} Mode0Prepared;

#ifdef FRLG_GBA_MODE0_REFERENCE_CANDIDATE
bool frlg_gba_mode0_reference_validated(const FrlgGbaMemory *memory,
                                        FrlgRgb8 *output, const uint16_t *bldy_by_line);
#endif

static bool prepare_mode0(const FrlgGbaMemory *memory,
                         const FrlgGbaDisplaySnapshot *display,
                         FrlgRgb8 *output, size_t output_pixels,
                         Mode0Prepared *prepared)
{
    unsigned int bg;
    if (!memory || !display || !output || !prepared || output_pixels < FRLG_GBA_SCREEN_PIXELS)
        return false;
    if (display->forced_blank)
        return true;
    if (display->mode != 0)
        return false;

    const bool win0 = (display->control & 0x2000) != 0;
    const bool win1 = (display->control & 0x4000) != 0;
    const bool objwin = (display->control & 0x8000) != 0;
    unsigned int *win_x1 = prepared->win_x1, *win_x2 = prepared->win_x2;
    unsigned int *win_y1 = prepared->win_y1, *win_y2 = prepared->win_y2;
    unsigned int *win_inside = prepared->win_inside;
    win_inside[0] = win_inside[1] = 0x3f;
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
        /* Every 10-bit tile index fits when even the last tile stays in VRAM. */
        if (chars + 1024 * tile_bytes <= 65536)
            continue;
        for (unsigned int entry = 0; entry < blocks * 1024; entry++)
            if (chars + (read16(memory->vram + map + entry * 2) & 1023) * tile_bytes + tile_bytes > 65536)
                return false;
    }
    BgInfo *backgrounds = prepared->backgrounds;
    uint8_t (*priority_layers)[4] = prepared->priority_layers;
    unsigned int *priority_counts = prepared->priority_counts;
    for (bg = 0; bg < 4; bg++)
    {
        if (display->control & (0x100u << bg))
        {
            const unsigned int control = display->background_control[bg];
            const unsigned int priority = control & 3;
            priority_layers[priority][priority_counts[priority]++] = (uint8_t)bg;
            backgrounds[bg] = (BgInfo){.control = control,
                                       .x_scroll = display->background_x[bg],
                                       .y_scroll = display->background_y[bg],
                                       .x_mask = (control & 0x4000) ? 511 : 255,
                                       .y_mask = (control & 0x8000) ? 511 : 255,
                                       .map = ((control >> 8) & 31) * 2048,
                                       .chars = ((control >> 2) & 3) * 16384,
                                       .row_stride = (control & 0x4000) ? 4096 : 2048};
        }
    }
    ObjInfo *objects = prepared->objects;
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
            int ox = attr1 & 511;
            int oy = attr0 & 255;
            if (ox >= 256)
                ox -= 512;
            if (oy >= 160)
                oy -= 256;
            const bool affine = (attr0 & 0x100) != 0;
            const unsigned int draw_scale = affine && (attr0 & 0x200) ? 2 : 1;
            objects[i] = (ObjInfo){.attr0 = (uint16_t)attr0, .attr1 = (uint16_t)attr1,
                                   .attr2 = (uint16_t)attr2, .width = (uint8_t)width,
                                   .height = (uint8_t)height, .ox = (int16_t)ox,
                                   .oy = (int16_t)oy, .draw_width = (uint8_t)(width * draw_scale),
                                   .draw_height = (uint8_t)(height * draw_scale)};
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
    prepared->win0 = win0;
    prepared->win1 = win1;
    prepared->objwin = objwin;
    prepared->win_outside = win_outside;
    prepared->win_obj = win_obj;
    prepared->bldcnt = read16(memory->io + 0x50);
    prepared->blend_mode = (prepared->bldcnt >> 6) & 3;
    const unsigned int alpha = read16(memory->io + 0x52);
    prepared->eva = limited_coefficient(alpha & 31);
    prepared->evb = limited_coefficient((alpha >> 8) & 31);
    prepared->uniform_ey = limited_coefficient(read16(memory->io + 0x54) & 31);
    return true;
}

static bool render_mode0(const FrlgGbaMemory *memory,
                         const FrlgGbaDisplaySnapshot *display,
                         FrlgRgb8 *output, size_t output_pixels,
                         const uint16_t *bldy_by_line)
{
    Mode0Prepared prepared = {0};
    if (!prepare_mode0(memory, display, output, output_pixels, &prepared))
        return false;
    if (display->forced_blank)
    {
        for (unsigned int pixel = 0; pixel < FRLG_GBA_SCREEN_PIXELS; pixel++)
            output[pixel] = (FrlgRgb8){255, 255, 255};
        return true;
    }
#ifdef FRLG_GBA_MODE0_REFERENCE_CANDIDATE
    return frlg_gba_mode0_reference_validated(memory, output, bldy_by_line);
#else
    unsigned int bg;
    const bool win0 = prepared.win0, win1 = prepared.win1, objwin = prepared.objwin;
    const unsigned int *win_x1 = prepared.win_x1, *win_x2 = prepared.win_x2;
    const unsigned int *win_y1 = prepared.win_y1, *win_y2 = prepared.win_y2;
    const unsigned int *win_inside = prepared.win_inside;
    const unsigned int win_outside = prepared.win_outside, win_obj = prepared.win_obj;
    BgInfo *backgrounds = prepared.backgrounds;
    const uint8_t (*priority_layers)[4] = prepared.priority_layers;
    const unsigned int *priority_counts = prepared.priority_counts;
    const ObjInfo *objects = prepared.objects;
    const unsigned int bldcnt = prepared.bldcnt, mode = prepared.blend_mode;
    const unsigned int eva = prepared.eva, evb = prepared.evb, uniform_ey = prepared.uniform_ey;
    for (unsigned int y = 0; y < FRLG_GBA_SCREEN_HEIGHT; y++)
    {
    for (bg = 0; bg < 4; bg++)
    {
        if (!(display->control & (0x100u << bg)))
            continue;
        BgInfo *background = backgrounds + bg;
        const unsigned int sy = (y + background->y_scroll) & background->y_mask;
        background->row_map = background->map + (sy / 256) * background->row_stride;
        background->tile_row = (sy / 8 % 32) * 64;
        background->tile_y = sy & 7;
    }
    uint8_t line_objects[128];
    unsigned int line_count = 0;
    if (display->control & FRLG_GBA_DISPCNT_OBJ)
    {
        for (unsigned int i = 0; i < 128; i++)
        {
            const ObjInfo *object = objects + i;
            if (!object->width)
                continue;
            if ((int)y >= object->oy && (int)y < object->oy + object->draw_height)
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
            for (unsigned int candidate = 0; candidate < priority_counts[priority]; candidate++)
            {
                const unsigned int layer = priority_layers[priority][candidate];
                if (!(window_mask & (1u << layer)))
                    continue;
                const unsigned int index = background_palette_index(memory, backgrounds + layer, x);
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
#endif
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
