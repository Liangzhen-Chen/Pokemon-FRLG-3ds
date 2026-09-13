#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "global.h"
#include "task.h"
#include "scanline_effect.h"
#include "frlg_native_scanline.h"

static FrlgGbaMemory memory;
static int destroyed_task = -1;

void DestroyTask(u8 taskId)
{
    destroyed_task = taskId;
}

static void expect_inactive(void)
{
    uint16_t copied[160];
    bool active = true;
    memset(copied, 0x5a, sizeof(copied));
    assert(frlg_native_scanline_copy_frame(copied, &active));
    assert(!active && copied[0] == 0x5a5a && copied[159] == 0x5a5a);
}

int main(int argc, char **argv)
{
    uint16_t copied[160];
    bool active = false;
    struct ScanlineEffectParams params;
    assert(frlg_native_io_bind(&memory));
    params = (struct ScanlineEffectParams){(volatile void *)REG_ADDR_BLDY,
                                           SCANLINE_EFFECT_DMACNT_16BIT, 1, 0};
    ScanlineEffect_SetParams(params);
    assert(frlg_native_scanline_status() == FRLG_NATIVE_SCANLINE_OK);
    assert(gScanlineEffect.srcBuffer == 0 && gScanlineEffect.state == 1);
    for (unsigned int y = 0; y < 160; y++)
    {
        gScanlineEffectRegBuffers[0][y] = (u16)(y % 17);
        gScanlineEffectRegBuffers[1][y] = (u16)(16 - y % 17);
    }
    frlg_native_scanline_begin_frame();
    expect_inactive();
    ScanlineEffect_InitHBlankDmaTransfer();
    assert(gScanlineEffect.srcBuffer == 1);
    assert(REG_BLDY == 0);
    assert(frlg_native_scanline_copy_frame(copied, &active) && active);
    for (unsigned int y = 0; y < 160; y++)
        assert(copied[y] == y % 17);
    gScanlineEffectRegBuffers[0][1] = 15;
    assert(frlg_native_scanline_copy_frame(copied, &active) && active && copied[1] == 1);

    frlg_native_scanline_begin_frame();
    expect_inactive();
    ScanlineEffect_InitHBlankDmaTransfer();
    assert(gScanlineEffect.srcBuffer == 0);
    assert(REG_BLDY == 16);
    assert(frlg_native_scanline_copy_frame(copied, &active) && active);
    for (unsigned int y = 0; y < 160; y++)
        assert(copied[y] == 16 - y % 17);

    gScanlineEffect.state = 3;
    ScanlineEffect_InitHBlankDmaTransfer();
    assert(gScanlineEffect.state == 0);
    expect_inactive();
    ScanlineEffect_InitHBlankDmaTransfer();
    expect_inactive();

    ScanlineEffect_SetParams(params);
    frlg_native_scanline_begin_frame();
    ScanlineEffect_InitHBlankDmaTransfer();
    gScanlineEffect.waveTaskId = 7;
    ScanlineEffect_Stop();
    assert(destroyed_task == 7 && gScanlineEffect.waveTaskId == 0xff);
    assert(gScanlineEffect.state == 0);
    expect_inactive();

    if (argc == 2)
    {
        FrlgNativeScanlineStatus expected;
        if (!strcmp(argv[1], "dest"))
        {
            params.dmaDest = (volatile void *)REG_ADDR_BG0HOFS;
            expected = FRLG_NATIVE_SCANLINE_UNSUPPORTED_DEST;
        }
        else if (!strcmp(argv[1], "control"))
        {
            params.dmaControl = SCANLINE_EFFECT_DMACNT_32BIT;
            expected = FRLG_NATIVE_SCANLINE_UNSUPPORTED_CONTROL;
        }
        else
        {
            assert(!strcmp(argv[1], "state"));
            params.initState = 2;
            expected = FRLG_NATIVE_SCANLINE_UNSUPPORTED_STATE;
        }
        ScanlineEffect_SetParams(params);
        assert(frlg_native_scanline_status() == expected);
        frlg_native_scanline_begin_frame();
        ScanlineEffect_Stop();
        assert(frlg_native_scanline_status() == expected);
        assert(!frlg_native_scanline_copy_frame(copied, &active));
    }
    return 0;
}
