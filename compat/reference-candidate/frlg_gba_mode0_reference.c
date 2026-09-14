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
        const uint8_t red = (uint8_t)(color & 255u) >> 3;
        const uint8_t green = (uint8_t)((color >> 8) & 255u) >> 3;
        const uint8_t blue = (uint8_t)((color >> 16) & 255u) >> 3;
        output[pixel] = (FrlgRgb8){
            (uint8_t)((red << 3) | (red >> 2)),
            (uint8_t)((green << 3) | (green >> 2)),
            (uint8_t)((blue << 3) | (blue >> 2))
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
