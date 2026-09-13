#include <assert.h>
#include <string.h>
#include "frlg_native_io.h"

int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    _Alignas(4) uint8_t source[96], dest[96];
    for (unsigned i=0;i<96;i++) source[i]=(uint8_t)(i+1);
    memset(dest,0,sizeof(dest));
    assert(frlg_native_io_bind(&memory));
    assert(frlg_native_cpu_set(source,dest,3,false));
    assert(!memcmp(source,dest,6) && dest[6]==0);
    CpuSet(source,dest,0x04000000|3);
    assert(!memcmp(source,dest,12) && dest[12]==0);
    CpuSet(source,(void *)(uintptr_t)FRLG_GBA_PALETTE_BASE,0x01000000|4);
    for(unsigned i=0;i<8;i+=2) assert(memory.palette[i]==1 && memory.palette[i+1]==2);
    CpuSet((void *)(uintptr_t)FRLG_GBA_PALETTE_BASE,dest,0x05000000|3);
    for(unsigned i=0;i<12;i+=4) assert(!memcmp(dest+i,memory.palette,4));
    memset(dest,0,sizeof(dest));
    CpuFastSet(source,dest,9); /* rounds to two eight-word blocks */
    assert(!memcmp(source,dest,64) && dest[64]==0);
    CpuFastSet(source,dest,0x01000000|1);
    for(unsigned i=0;i<32;i+=4) assert(!memcmp(source,dest+i,4));
    /* Fast copy loads an entire block before storing, unlike unit-wise DMA. */
    memcpy(dest,source,sizeof(dest));
    CpuFastSet(dest,dest+4,9);
    assert(!memcmp(dest+4,source,32));
    assert(!memcmp(dest+36,source+28,4));
    assert(!memcmp(dest+40,source+36,28));
    assert(frlg_native_cpu_set(NULL,NULL,0,false));
    assert(frlg_native_cpu_set(NULL,NULL,0x01000000,true));
    assert(!frlg_native_cpu_set(source+1,dest,1,false));
    assert(!frlg_native_cpu_set(source,dest+2,1,true));
    assert(!frlg_native_cpu_set(NULL,dest,1,false));
    void *edge=(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+FRLG_GBA_VRAM_SIZE-16);
    assert(!frlg_native_cpu_set(source,edge,1,true));
    assert(memory.vram[FRLG_GBA_VRAM_SIZE-16]==0);
    memset(dest,0xa5,sizeof(dest));
    assert(!frlg_native_cpu_set(edge,dest,8,true));
    for(unsigned i=0;i<96;i++) assert(dest[i]==0xa5);
    /* Fixed source is only one word, even for a large fill. */
    void *last=(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+FRLG_GBA_VRAM_SIZE-4);
    assert(frlg_native_fill(0x11223344,last,4,4));
    assert(frlg_native_cpu_set(last,dest,0x01000000|8,true));
    for(unsigned i=0;i<32;i+=4) assert(dest[i]==0x44 && dest[i+3]==0x11);
    return 0;
}
