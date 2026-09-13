#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "frlg_native_title_assets.h"

int main(int argc, char **argv)
{
    _Alignas(4) static FrlgGbaMemory memory;
    _Alignas(4) static uint8_t decoded[FRLG_GBA_VRAM_SIZE];
    static uint8_t expected[FRLG_GBA_VRAM_SIZE];
    assert(gFrlgTitleLzResourceCount == 7 && argc == 8);
    assert(frlg_native_io_bind(&memory));
    assert(frlg_native_lz_resources_bind(gFrlgTitleLzResources,gFrlgTitleLzResourceCount));
    for (size_t i=0; i<gFrlgTitleLzResourceCount; i++) {
        const FrlgNativeLzResource *resource = &gFrlgTitleLzResources[i];
        assert(resource->size >= 4);
        size_t size = resource->data[1] | ((size_t)resource->data[2]<<8) | ((size_t)resource->data[3]<<16);
        assert(size && size <= sizeof(decoded));
        FILE *file = fopen(argv[i+1],"rb");
        assert(file);
        assert(fread(expected,1,sizeof(expected),file) == size);
        assert(!ferror(file) && fgetc(file) == EOF);
        fclose(file);
        memset(decoded,0xA5,sizeof(decoded));
        LZ77UnCompWram(resource->data,decoded);
        assert(!memcmp(decoded,expected,size));
    }
    puts("Seven linked title LZ resources match original uncompressed bytes.");
    return 0;
}
