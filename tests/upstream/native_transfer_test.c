#include <assert.h>
#include <string.h>
#include "frlg_native_io.h"

int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    _Alignas(4) uint8_t source[16] = {1,2,3,4,5,6,7,8};
    _Alignas(4) uint8_t dest[16] = {0};
    assert(frlg_native_io_bind(&memory));
    assert(frlg_native_copy(source,(void *)(uintptr_t)FRLG_GBA_VRAM_BASE,8,4));
    assert(!memcmp(source,memory.vram,8));
    assert(frlg_native_copy((void *)(uintptr_t)FRLG_GBA_VRAM_BASE,dest,8,2));
    assert(!memcmp(source,dest,8));
    assert(frlg_native_fill(0x12345678,(void *)(uintptr_t)FRLG_GBA_PALETTE_BASE,8,2));
    for (unsigned i=0;i<8;i+=2) assert(memory.palette[i]==0x78 && memory.palette[i+1]==0x56);
    assert(frlg_native_fill(0x12345678,dest,8,4));
    const uint8_t expected[8] = {0x78,0x56,0x34,0x12,0x78,0x56,0x34,0x12};
    assert(!memcmp(dest,expected,8));
    assert(!frlg_native_copy(source,(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+FRLG_GBA_VRAM_SIZE-4),8,4));
    assert(memory.vram[FRLG_GBA_VRAM_SIZE-4]==0);
    assert(!frlg_native_copy((void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+FRLG_GBA_VRAM_SIZE-4),dest,8,4));
    assert(!memcmp(dest,expected,8));
    assert(!frlg_native_fill(0, (void *)(uintptr_t)0x06018000,4,4));
    assert(!frlg_native_copy(source+1,dest,4,2));
    assert(!frlg_native_fill(0,dest,3,2));
    assert(!frlg_native_copy(source,dest,4,1));
    assert(!frlg_native_copy(NULL,dest,4,4));
    assert(frlg_native_copy(NULL,NULL,0,4));
    assert(frlg_native_fill(0,NULL,0,2));
    /* DMA reads and writes successive units, not memmove semantics. */
    _Alignas(4) uint8_t overlap[8]={1,2,3,4,5,6,7,8};
    assert(frlg_native_copy(overlap,overlap+2,6,2));
    for (unsigned i=0;i<8;i+=2) assert(overlap[i]==1 && overlap[i+1]==2);
    return 0;
}
