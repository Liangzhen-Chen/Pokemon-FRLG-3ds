#include <assert.h>
#include <string.h>
#include "frlg_native_io.h"

int main(void)
{
    _Alignas(4) static FrlgGbaMemory memory;
    static const uint8_t stream[]={0x10,8,0,0,0x20,'A','B',0x30,1};
    static const FrlgNativeLzResource resources[]={{stream,sizeof(stream)}};
    assert(frlg_native_io_bind(&memory));
    assert(frlg_native_lz_resources_bind(resources,1));
    assert(!frlg_native_lz_resources_bind(NULL,1));
    const FrlgNativeLzResource duplicates[]={{stream,sizeof(stream)},{stream,sizeof(stream)}};
    assert(!frlg_native_lz_resources_bind(duplicates,2));
    const FrlgNativeLzResource bad[]={{NULL,9}};
    assert(!frlg_native_lz_resources_bind(bad,1));
    LZ77UnCompVram(stream,(void *)(uintptr_t)FRLG_GBA_VRAM_BASE);
    assert(!memcmp(memory.vram,"ABABABAB",8));
    uint8_t native[8]={0};
    LZ77UnCompWram(stream,native);
    assert(!memcmp(native,"ABABABAB",8));
    assert(!frlg_native_lz77(stream,sizeof(stream),(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+FRLG_GBA_VRAM_SIZE-4),true));
    assert(memory.vram[FRLG_GBA_VRAM_SIZE-4]==0);
    assert(!frlg_native_lz77(stream,sizeof(stream),(void *)(uintptr_t)(FRLG_GBA_VRAM_BASE+1),true));
    assert(!frlg_native_lz77(stream,3,native,false));
    assert(frlg_native_lz_resources_bind(NULL,0));
    /* Checked API remains usable without an ABI table. */
    assert(frlg_native_lz77(stream,sizeof(stream),native,false));
    return 0;
}
