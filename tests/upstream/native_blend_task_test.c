#include <assert.h>
#include <string.h>
#include "global.h"
#include "gflib.h"
#include "task.h"
#include "menu.h"
#include "frlg_gba_mode0.h"

static unsigned phase;
static void before(u8 id) { (void)id; assert(phase == 0); phase = 1; }
static void after(u8 id) { (void)id; assert(phase == 1); phase = 2; }

int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    static FrlgRgb8 pixels[FRLG_GBA_SCREEN_PIXELS];
    FrlgGbaDisplaySnapshot display;
    static const u16 expected[] = {0x1004,0x0C04,0x0C08,0x0808,0x080C,0x040C,0x0410,0x0010};
    assert(frlg_native_io_bind(&memory));
    InitGpuRegManager();
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_PALETTE_BASE + 2, 0x001f));
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_PALETTE_BASE + 34, 0x03e0));
    memset(memory.vram + 32, 0x11, 32);
    memset(memory.vram + 16384 + 32, 0x11, 32);
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_VRAM_BASE + 16 * 2048, 1));
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_VRAM_BASE + 20 * 2048, 0x1001));
    SetGpuReg(REG_OFFSET_DISPCNT, 0x300);
    SetGpuReg(REG_OFFSET_BG0CNT, 16 << 8);
    SetGpuReg(REG_OFFSET_BG1CNT, (20 << 8) | 4 | 1);
    SetGpuReg(REG_OFFSET_BLDCNT, 0x0241);
    ResetTasks();
    assert(!IsBlendTaskActive());
    u8 first = CreateTask(before,0);
    u8 last = CreateTask(after,10);
    StartBlendTask(0,16,16,0,4,5);
    assert(IsBlendTaskActive() && GetTaskCount() == 3);
    assert(GetGpuReg(REG_OFFSET_BLDALPHA) == 0x1000);
    for (unsigned i=0; i<8; i++) {
        phase = 0;
        RunTasks();
        assert(phase == 2);
        assert(GetGpuReg(REG_OFFSET_BLDALPHA) == expected[i]);
        CopyBufferedValuesToGpuRegs();
        assert(REG_BLDALPHA == expected[i]);
        if (i == 3) {
            assert(frlg_gba_display_snapshot(&memory, &display));
            assert(frlg_gba_mode0_render(&memory, &display, pixels, FRLG_GBA_SCREEN_PIXELS));
            assert(pixels[0].red == 123 && pixels[0].green == 123 && pixels[0].blue == 0);
        }
        assert(IsBlendTaskActive() == (i < 7));
    }
    assert(GetTaskCount() == 2);
    DestroyTask(first);
    DestroyTask(last);
    assert(GetTaskCount() == 0);
    StartBlendTask(16,0,0,16,1,0);
    RunTasks();
    assert(IsBlendTaskActive());
    RunTasks();
    assert(!IsBlendTaskActive() && GetGpuReg(REG_OFFSET_BLDALPHA) == 0x1000);
    /* Exact durations used by the original intro, including fractional deltas. */
    const u8 durations[] = {48,16,20};
    for (unsigned i=0; i<3; i++) {
        bool reverse = i == 2;
        StartBlendTask(reverse ? 16 : 0,reverse ? 0 : 16,
                       reverse ? 0 : 16,reverse ? 16 : 0,durations[i],0);
        unsigned frames = 0;
        while (IsBlendTaskActive() && frames < 100) {
            RunTasks();
            frames++;
        }
        assert(frames == 2u*durations[i] && !IsBlendTaskActive());
        assert(GetGpuReg(REG_OFFSET_BLDALPHA) == (reverse ? 0x1000 : 0x0010));
    }
    return 0;
}
