#include <assert.h>
#include "global.h"
#include "palette.h"
#ifdef INCBIN_U8
#error Fake asset macros must not be available
#endif
int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    assert(frlg_native_io_bind(&memory));
    assert(frlg_native_memory()==&memory);
    assert(REG_BASE==(uintptr_t)memory.io);
    assert(PLTT==(uintptr_t)memory.palette);
    REG_DISPCNT=0x0100;
    assert(memory.io[0]==0 && memory.io[1]==1);
    DmaFill16(3,0xabcd,(void *)(uintptr_t)FRLG_GBA_PALETTE_BASE,4);
    assert(memory.palette[0]==0xcd && memory.palette[1]==0xab);
    *(vu16 *)PLTT=0x1234;
    assert(memory.palette[0]==0x34 && memory.palette[1]==0x12);
    assert(sizeof(struct PaletteFadeControl)>0);
    /* Original startup code passes integer address macros, not void pointers. */
    DmaFill16(3,0x2211,FRLG_GBA_VRAM_BASE,4);
    DmaFill32(3,0x66554433,FRLG_GBA_OAM_BASE,4);
    DmaCopy16(3,FRLG_GBA_VRAM_BASE,FRLG_GBA_EWRAM_BASE,4);
    DmaCopy32(3,FRLG_GBA_OAM_BASE,FRLG_GBA_IWRAM_BASE,4);
    assert(memory.ewram[0]==0x11 && memory.ewram[1]==0x22);
    assert(memory.iwram[0]==0x33 && memory.iwram[3]==0x66);
    return 0;
}
