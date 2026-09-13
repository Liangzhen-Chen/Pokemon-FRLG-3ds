#include <assert.h>
#include "global.h"
#include "palette.h"
#include "gpu_regs.h"
#include "frlg_gba_mode0.h"

int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    static FrlgRgb8 pixels[FRLG_GBA_SCREEN_PIXELS];
    FrlgGbaDisplaySnapshot display;
    _Alignas(4) u16 colors[32];
    assert(frlg_native_io_bind(&memory));
    assert(PLTT==(uintptr_t)memory.palette && REG_BASE==(uintptr_t)memory.io);
    InitGpuRegManager();
    REG_VCOUNT=161;
    SetGpuReg(REG_OFFSET_DISPCNT,DISPCNT_BG0_ON);
    ResetPaletteFade();
    for(unsigned i=0;i<32;i++) colors[i]=i<16 ? RGB_RED : RGB_GREEN;
    LoadPalette(colors,0,sizeof(colors));
    TransferPlttBuffer();
    assert(memory.palette[0]==31 && memory.palette[1]==0);
    assert(BeginNormalPaletteFade(1,0,16,0,RGB_WHITE));
    assert(!BeginNormalPaletteFade(1,0,0,16,RGB_BLACK));
    assert(gPlttBufferFaded[0]==RGB_WHITE);
    assert(gPlttBufferFaded[16]==RGB_GREEN);
    assert(frlg_gba_display_snapshot(&memory,&display));
    assert(frlg_gba_mode0_render(&memory,&display,pixels,FRLG_GBA_SCREEN_PIXELS));
    assert(pixels[0].red==255 && pixels[0].green==255 && pixels[0].blue==255);
    UpdatePaletteFade();
    assert(UpdatePaletteFade()==PALETTE_FADE_STATUS_LOADING);
    TransferPlttBuffer();
    unsigned frames=0;
    while(gPaletteFade.active && frames<100) {
        UpdatePaletteFade();
        TransferPlttBuffer();
        frames++;
    }
    assert(frames>1 && frames<100 && !gPaletteFade.active);
    assert(UpdatePaletteFade()==PALETTE_FADE_STATUS_DONE);
    assert(gPlttBufferFaded[0]==RGB_RED && gPlttBufferFaded[16]==RGB_GREEN);
    assert(frlg_gba_mode0_render(&memory,&display,pixels,FRLG_GBA_SCREEN_PIXELS));
    assert(pixels[0].red==255 && pixels[0].green==0 && pixels[0].blue==0);
    /* Original direct palette reads must use the same backing memory. */
    assert(frlg_gba_memory_write16(&memory,FRLG_GBA_PALETTE_BASE+2,RGB_BLUE));
    ReadPlttIntoBuffers();
    assert(gPlttBufferFaded[1]==RGB_BLUE && gPlttBufferUnfaded[1]==RGB_BLUE);
    FillPalette(RGB_BLUE,0,2);
    gPaletteFade.bufferTransferDisabled=TRUE;
    TransferPlttBuffer();
    assert(memory.palette[0]==31 && memory.palette[1]==0);
    gPaletteFade.bufferTransferDisabled=FALSE;
    TransferPlttBuffer();
    assert(memory.palette[0]==0 && memory.palette[1]==0x7c);
    FillPalette(RGB_GREEN,OBJ_PLTT_OFFSET,32);
    assert(BeginNormalPaletteFade(PALETTES_ALL,0,0,16,RGB_BLACK));
    frames=0;
    while(gPaletteFade.active && frames<100) {
        UpdatePaletteFade();
        TransferPlttBuffer();
        frames++;
    }
    assert(frames<100 && !gPaletteFade.active);
    assert(gPlttBufferFaded[0]==RGB_BLACK && gPlttBufferFaded[OBJ_PLTT_OFFSET]==RGB_BLACK);
    return 0;
}
