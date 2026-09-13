#include <assert.h>
#include <string.h>
#include "global.h"
#include "dma3.h"

int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    _Alignas(4) static uint8_t source[8192];
    assert(frlg_native_io_bind(&memory));
    for(unsigned i=0;i<sizeof(source);i++) source[i]=(uint8_t)(i*13+1);
    ClearDma3Requests();
    REG_VCOUNT=161;
    s16 request=RequestDma3Copy(source,(void *)(uintptr_t)FRLG_GBA_VRAM_BASE,4098,DMA3_16BIT);
    assert(request>=0 && WaitDma3Request(request)==-1);
    assert(memory.vram[0]==0);
    ProcessDma3Requests();
    assert(WaitDma3Request(request)==0 && !memcmp(memory.vram,source,4098));
    request=RequestDma3Copy(source,(void *)(uintptr_t)FRLG_GBA_EWRAM_BASE,8192,DMA3_32BIT);
    REG_VCOUNT=225; ProcessDma3Requests();
    assert(WaitDma3Request(request)==-1 && memory.ewram[0]==0);
    REG_VCOUNT=224; ProcessDma3Requests();
    assert(WaitDma3Request(request)==0 && !memcmp(memory.ewram,source,8192));
    assert(RequestDma3Fill(0x12345678,(void *)(uintptr_t)FRLG_GBA_VRAM_BASE,8192,DMA3_32BIT)>=0);
    ProcessDma3Requests();
    for(unsigned i=0;i<8192;i+=4) {
        assert(memory.vram[i]==0x78 && memory.vram[i+1]==0x56);
        assert(memory.vram[i+2]==0x34 && memory.vram[i+3]==0x12);
    }
    assert(RequestDma3Fill(0x1234abcd,(void *)(uintptr_t)FRLG_GBA_PALETTE_BASE,512,DMA3_16BIT)>=0);
    ProcessDma3Requests();
    for(unsigned i=0;i<512;i+=2) assert(memory.palette[i]==0xcd && memory.palette[i+1]==0xab);
    /* Preserve the original queue's per-call 40 KiB budget. */
    ClearDma3Requests();
    assert(RequestDma3Fill(0x11111111,(void *)(uintptr_t)FRLG_GBA_VRAM_BASE,32768,DMA3_32BIT)>=0);
    request=RequestDma3Fill(0x22222222,(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+32768),16384,DMA3_32BIT);
    ProcessDma3Requests();
    assert(memory.vram[0]==0x11 && memory.vram[32768]==0 && WaitDma3Request(request)==-1);
    ProcessDma3Requests();
    assert(memory.vram[32768]==0x22 && WaitDma3Request(-1)==0);
    ClearDma3Requests();
    for(unsigned i=0;i<128;i++)
        assert(RequestDma3Fill(i,(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+i*4),4,DMA3_32BIT)>=0);
    assert(RequestDma3Fill(0,(void *)(uintptr_t)FRLG_GBA_VRAM_BASE,4,DMA3_32BIT)==-1);
    ProcessDma3Requests();
    assert(WaitDma3Request(-1)==0);
    for(unsigned i=0;i<128;i++) assert(memory.vram[i*4]==i);
    return 0;
}
