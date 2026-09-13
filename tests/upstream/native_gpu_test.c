#include <assert.h>
#include <string.h>
#include "global.h"
#include "frlg_gba_mode0.h"
#include "dma3.h"

/* Selected upstream ABI; avoid including its full game-global header. */
void InitGpuRegManager(void);
void SetGpuReg(u8 offset, u16 value);
u16 GetGpuReg(u8 offset);
void CopyBufferedValuesToGpuRegs(void);
void SetGpuRegBits(u8 offset, u16 mask);
void ClearGpuRegBits(u8 offset, u16 mask);
void EnableInterrupts(u16 mask);
void DisableInterrupts(u16 mask);

int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    static FrlgRgb8 pixels[FRLG_GBA_SCREEN_PIXELS];
    FrlgGbaDisplaySnapshot display;
    assert(frlg_native_io_bind(&memory));
    /* Check before any register dereference: failed mapping must not hit GBA IO. */
    assert(REG_BASE == (uintptr_t)memory.io);
    InitGpuRegManager();
    SetGpuReg(REG_OFFSET_DISPCNT, FRLG_GBA_DISPCNT_BG0);
    SetGpuReg(REG_OFFSET_BG0CNT, 16 << 8);
    assert(GetGpuReg(REG_OFFSET_DISPCNT) == FRLG_GBA_DISPCNT_BG0);
    assert(REG_DISPCNT == 0); /* active scanline: deferred */
    SetGpuReg(REG_OFFSET_BG0HOFS, 3);
    SetGpuReg(REG_OFFSET_BG0HOFS, 5); /* same queued register: newest value */
    CopyBufferedValuesToGpuRegs();
    assert(REG_DISPCNT == FRLG_GBA_DISPCNT_BG0 && REG_BG0HOFS == 5);
    REG_VCOUNT = 161;
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    assert(REG_BG0HOFS == 0); /* blanking scanline: immediate */
    SetGpuRegBits(REG_OFFSET_BG0CNT, 3);
    assert(REG_BG0CNT == ((16 << 8) | 3));
    ClearGpuRegBits(REG_OFFSET_BG0CNT, 3);
    assert(REG_BG0CNT == (16 << 8));
    REG_IME = 1;
    EnableInterrupts(INTR_FLAG_VBLANK | INTR_FLAG_HBLANK);
    assert(REG_IE == 3 && REG_IME == 1);
    assert((REG_DISPSTAT & 0x18) == 0x18);
    DisableInterrupts(INTR_FLAG_HBLANK);
    assert(REG_IE == 1 && (REG_DISPSTAT & 0x18) == 8);
    /* Real upstream writes now drive the existing software renderer. */
    /* Original CPU macros prepare data; original DMA queue uploads it. */
    _Alignas(4) uint8_t tile[32];
    CpuFastFill8(0x11,tile,sizeof(tile));
    CpuFill16(0x001f,(void *)(uintptr_t)(FRLG_GBA_PALETTE_BASE+2),2);
    ClearDma3Requests();
    assert(RequestDma3Copy(tile,(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+32),32,DMA3_32BIT)>=0);
    assert(RequestDma3Fill(1,(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+16*2048),2,DMA3_16BIT)>=0);
    ProcessDma3Requests();
    assert(WaitDma3Request(-1)==0);
    assert(frlg_gba_display_snapshot(&memory,&display));
    assert(frlg_gba_mode0_render(&memory,&display,pixels,FRLG_GBA_SCREEN_PIXELS));
    assert(pixels[0].red == 255 && pixels[0].green == 0 && pixels[0].blue == 0);
    assert(pixels[8].red == 0);
    _Alignas(4) const u16 green=0x03e0;
    CpuCopy16(&green,(void *)(uintptr_t)(FRLG_GBA_PALETTE_BASE+2),2);
    assert(frlg_gba_mode0_render(&memory,&display,pixels,FRLG_GBA_SCREEN_PIXELS));
    assert(pixels[0].red==0 && pixels[0].green==255 && pixels[0].blue==0);
    return 0;
}
