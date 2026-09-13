#include <assert.h>
#include <string.h>
#include "global.h"
#include "gflib.h"
#include "new_menu_helpers.h"

extern bool32 CheckHeap(void);
void AGBAssert(const char *file, int line, const char *expression, int stop)
{
    (void)file; (void)line; (void)expression; (void)stop;
    assert(!"unexpected original allocator assertion");
}

int main(void)
{
    _Alignas(8) static FrlgGbaMemory memory;
    static const u8 compressed[] = {0x10,8,0,0,0x20,'A','B',0x30,1};
    static const FrlgNativeLzResource resources[] = {{compressed,sizeof(compressed)}};
    const struct BgTemplate bg = {.bg=0, .charBaseIndex=0, .mapBaseIndex=31};
    assert(frlg_native_io_bind(&memory));
    assert(frlg_native_lz_resources_bind(resources,1));
    InitHeap(gHeap,HEAP_SIZE);
    InitGpuRegManager();
    ClearDma3Requests();
    ResetBgsAndClearDma3BusyFlags(FALSE);
    InitBgsFromTemplates(0,&bg,1);
    ResetTempTileDataBuffers();
    void *tiles = DecompressAndCopyTileDataToVram(0,compressed,0,1,0);
    void *map = DecompressAndCopyTileDataToVram(0,compressed,0,2,1);
    assert(tiles && map && tiles != map && CheckHeap());
    assert(!memcmp(tiles,"ABABABAB",8));
    assert(FreeTempTileDataBuffersIfPossible());
    assert(memory.vram[32] == 0 && memory.vram[31*2048+64] == 0);
    REG_VCOUNT = 161;
    ProcessDma3Requests();
    assert(!memcmp(memory.vram+32,"ABABABAB",8));
    assert(!memcmp(memory.vram+31*2048+64,"ABABABAB",8));
    assert(!FreeTempTileDataBuffersIfPossible() && CheckHeap());
    assert(!FreeTempTileDataBuffersIfPossible());
    void *reused = Alloc(8);
    assert(reused == tiles);
    Free(reused);
    for (unsigned i = 0; i < 32; i++)
        assert(DecompressAndCopyTileDataToVram(0,compressed,0,i,0));
    assert(!DecompressAndCopyTileDataToVram(0,compressed,0,0,0));
    assert(FreeTempTileDataBuffersIfPossible());
    ProcessDma3Requests();
    assert(!FreeTempTileDataBuffersIfPossible());
    for (unsigned i = 0; i < 32; i++)
        assert(!memcmp(memory.vram+i*32,"ABABABAB",8));
    assert(CheckHeap());
    return 0;
}
