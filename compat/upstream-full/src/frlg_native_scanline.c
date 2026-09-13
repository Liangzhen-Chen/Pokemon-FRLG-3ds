#include "global.h"
#include "task.h"
#include "scanline_effect.h"
#include "frlg_native_scanline.h"
#include <string.h>

EWRAM_DATA u16 gScanlineEffectRegBuffers[2][0x3c0] = {0};
EWRAM_DATA struct ScanlineEffect gScanlineEffect = {.waveTaskId = 0xff};

static uint16_t s_frame_bldy[160];
static bool s_published;
static FrlgNativeScanlineStatus s_status;

static void set_first_scanline_reg(void)
{
    REG_BLDY = s_frame_bldy[0];
}

void frlg_native_scanline_begin_frame(void)
{
    s_published = false;
}

FrlgNativeScanlineStatus frlg_native_scanline_status(void)
{
    return s_status;
}

bool frlg_native_scanline_copy_frame(uint16_t out_bldy[160], bool *active)
{
    if (!out_bldy || !active || s_status != FRLG_NATIVE_SCANLINE_OK)
        return false;
    *active = s_published && gScanlineEffect.state == 1;
    if (*active)
        memcpy(out_bldy, s_frame_bldy, sizeof(s_frame_bldy));
    return true;
}

void ScanlineEffect_Stop(void)
{
    gScanlineEffect.state = 0;
    s_published = false;
    if (gScanlineEffect.waveTaskId != 0xff)
    {
        DestroyTask(gScanlineEffect.waveTaskId);
        gScanlineEffect.waveTaskId = 0xff;
    }
}

void ScanlineEffect_SetParams(struct ScanlineEffectParams params)
{
    if (s_status != FRLG_NATIVE_SCANLINE_OK)
        return;
    if ((uintptr_t)params.dmaDest != (uintptr_t)REG_ADDR_BLDY)
        s_status = FRLG_NATIVE_SCANLINE_UNSUPPORTED_DEST;
    else if (params.dmaControl != (u32)SCANLINE_EFFECT_DMACNT_16BIT)
        s_status = FRLG_NATIVE_SCANLINE_UNSUPPORTED_CONTROL;
    else if (params.initState != 1)
        s_status = FRLG_NATIVE_SCANLINE_UNSUPPORTED_STATE;
    if (s_status != FRLG_NATIVE_SCANLINE_OK)
    {
        s_published = false;
        gScanlineEffect.state = 0;
        return;
    }
    gScanlineEffect.dmaSrcBuffers[0] = gScanlineEffectRegBuffers[0] + 1;
    gScanlineEffect.dmaSrcBuffers[1] = gScanlineEffectRegBuffers[1] + 1;
    gScanlineEffect.dmaDest = params.dmaDest;
    gScanlineEffect.dmaControl = params.dmaControl;
    gScanlineEffect.setFirstScanlineReg = set_first_scanline_reg;
    gScanlineEffect.state = params.initState;
    gScanlineEffect.unused16 = params.unused9;
    gScanlineEffect.unused17 = params.unused9;
}

void ScanlineEffect_InitHBlankDmaTransfer(void)
{
    if (s_status != FRLG_NATIVE_SCANLINE_OK || gScanlineEffect.state == 0)
        return;
    if (gScanlineEffect.state == 3)
    {
        gScanlineEffect.state = 0;
        s_published = false;
        return;
    }
    if (gScanlineEffect.state != 1 || gScanlineEffect.srcBuffer > 1)
        s_status = FRLG_NATIVE_SCANLINE_UNSUPPORTED_STATE;
    else if ((uintptr_t)gScanlineEffect.dmaDest != (uintptr_t)REG_ADDR_BLDY)
        s_status = FRLG_NATIVE_SCANLINE_UNSUPPORTED_DEST;
    else if (gScanlineEffect.dmaControl != (u32)SCANLINE_EFFECT_DMACNT_16BIT)
        s_status = FRLG_NATIVE_SCANLINE_UNSUPPORTED_CONTROL;
    if (s_status != FRLG_NATIVE_SCANLINE_OK)
    {
        s_published = false;
        return;
    }
    memcpy(s_frame_bldy, gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer], sizeof(s_frame_bldy));
    gScanlineEffect.setFirstScanlineReg();
    s_published = true;
    gScanlineEffect.srcBuffer ^= 1;
}
