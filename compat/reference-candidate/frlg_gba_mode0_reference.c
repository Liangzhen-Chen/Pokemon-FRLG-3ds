#include "frlg_gba_mode0.h"

#ifdef FRLG_GBA_MODE0_REFERENCE_CANDIDATE
#include <string.h>

#ifdef TMC_3DS
#include <3ds.h>
#endif

#include "cpu/mode1.h"
#include "virtuappu.h"

static uint8_t io_copy[FRLG_GBA_IO_SIZE];
static uint16_t bg_palette[256], obj_palette[256], oam[512];
static const uint16_t *active_bldy;

#ifdef VIRTUAPPU_TESTING
static unsigned int window_normalization_count;

void frlg_gba_mode0_reference_reset_window_normalization_count(void)
{
    window_normalization_count = 0;
}

unsigned int frlg_gba_mode0_reference_get_window_normalization_count(void)
{
    return window_normalization_count;
}
#endif

static uint16_t read16(const uint8_t *bytes)
{
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static void write16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static bool objwin_is_horizontally_offscreen(const FrlgGbaMemory *memory, uint16_t dispcnt)
{
    static const uint8_t widths[3][4] = {
        {8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}
    };
    if (!(dispcnt & FRLG_GBA_DISPCNT_OBJ))
        return true;
    for (unsigned int i = 0; i < 128; i++)
    {
        const uint8_t *entry = memory->oam + i * 8;
        const uint16_t attr0 = read16(entry);
        const uint16_t attr1 = read16(entry + 2);
        const bool affine = (attr0 & 0x0100) != 0;
        if ((!affine && (attr0 & 0x0200)) || (attr0 & 0x0c00) != 0x0800)
            continue;
        const unsigned int shape = attr0 >> 14;
        if (shape >= 3)
            continue;
        int width = widths[shape][attr1 >> 14];
        if (affine && (attr0 & 0x0200))
            width *= 2;
        int x = attr1 & 0x01ff;
        if (x >= FRLG_GBA_SCREEN_WIDTH)
            x -= 512;
        if (x < FRLG_GBA_SCREEN_WIDTH && x + width > 0)
            return false;
    }
    return true;
}

static void normalize_inert_objwin(const FrlgGbaMemory *memory)
{
    const uint16_t dispcnt = read16(io_copy);
    const uint16_t windows = dispcnt & 0xe000;
    const uint16_t outside = read16(io_copy + 0x4a) & 0x003f;
    if (windows != 0x8000 || outside != 0x001f ||
        !objwin_is_horizontally_offscreen(memory, dispcnt))
        return;

    /* With no WIN0/WIN1, no OBJ-window object reaching the viewport, and an
     * outside mask that enables every color layer but disables effects, every
     * pixel uses the same no-effect controls. Express that equivalent state
     * directly so the reference renderer can use its direct no-effect path. */
    write16(io_copy, (uint16_t)(dispcnt & ~0x8000u));
    write16(io_copy + 0x50, 0);
#ifdef VIRTUAPPU_TESTING
    ++window_normalization_count;
#endif
}

#ifdef TMC_3DS
static bool runtime_ready, new3ds, core1_available;

bool Platform3DS_IsNew3DS(void) { return new3ds; }
bool Platform3DS_CanUseCore1(void) { return core1_available; }

static bool prepare_runtime(void)
{
    if (runtime_ready)
        return true;
    if (R_FAILED(APT_CheckNew3DS(&new3ds)))
        return false;
    if (new3ds)
        osSetSpeedupEnable(true);
    const u32 candidates[] = {80, 70, 50, 30};
    for (unsigned int i = 0; i < 4; i++)
    {
        u32 actual = 0;
        if (R_SUCCEEDED(APT_SetAppCpuTimeLimit(candidates[i])) &&
            R_SUCCEEDED(APT_GetAppCpuTimeLimit(&actual)) && actual > 0)
        {
            core1_available = true;
            break;
        }
    }
    runtime_ready = true;
    return true;
}
#else
static bool prepare_runtime(void) { return true; }
#endif

static void set_line_bldy(int line)
{
    const uint16_t value = active_bldy[line];
    io_copy[0x54] = (uint8_t)value;
    io_copy[0x55] = (uint8_t)(value >> 8);
}

bool frlg_gba_mode0_reference_validated(const FrlgGbaMemory *memory,
                                        FrlgRgb8 *output, const uint16_t *bldy_by_line)
{
    if (!prepare_runtime())
        return false;
    memcpy(io_copy, memory->io, sizeof(io_copy));
    normalize_inert_objwin(memory);
    memcpy(bg_palette, memory->palette, sizeof(bg_palette));
    memcpy(obj_palette, memory->palette + 512, sizeof(obj_palette));
    memcpy(oam, memory->oam, sizeof(oam));
    const VirtuaPPUMode1GbaMemory binding = {
        io_copy, (uint8_t *)memory->vram, bg_palette, obj_palette, oam
    };
    const PPUMemory ppu = {240, 160, 240, 1, 0};
    virtuappu_mode1_bind_gba_memory(&binding);
#ifdef TMC_3DS
    virtuappu_mode1_set_old3ds_profile(!new3ds);
#else
    virtuappu_mode1_set_old3ds_profile(false);
#endif
    virtuappu_mode1_set_color_correction(false);
    active_bldy = bldy_by_line;
    virtuappu_mode1_pre_line_callback = bldy_by_line ? set_line_bldy : NULL;
    virtuappu_mode1_render_frame(&ppu);
    virtuappu_mode1_pre_line_callback = NULL;
    active_bldy = NULL;
    for (unsigned int pixel = 0; pixel < FRLG_GBA_SCREEN_PIXELS; pixel++)
    {
        const uint32_t color = virtuappu_frame_buffer[pixel];
        const uint32_t expanded =
            (color & 0x00f8f8f8u) | ((color >> 5) & 0x00070707u);
        output[pixel] = (FrlgRgb8){
            (uint8_t)expanded,
            (uint8_t)(expanded >> 8),
            (uint8_t)(expanded >> 16)
        };
    }
    return true;
}

void frlg_gba_mode0_reference_shutdown(void)
{
    virtuappu_mode1_shutdown_workers();
    virtuappu_mode1_pre_line_callback = NULL;
    active_bldy = NULL;
#ifdef TMC_3DS
    runtime_ready = false;
    core1_available = false;
#endif
}
#endif
