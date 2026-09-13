#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frlg_native_io.h"

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *file=fopen(path,"rb");
    assert(file && fseek(file,0,SEEK_END)==0);
    long length=ftell(file);
    assert(length>0 && length<16*1024*1024 && fseek(file,0,SEEK_SET)==0);
    *size=(size_t)length;
    uint8_t *bytes=malloc(*size);
    assert(bytes && fread(bytes,1,*size,file)==*size);
    assert(fclose(file)==0);
    return bytes;
}

int main(int argc, char **argv)
{
    _Alignas(4) static FrlgGbaMemory memory;
    assert(argc>1 && (argc%2)==1 && frlg_native_io_bind(&memory));
    for(int i=1;i<argc;i+=2) {
        size_t packed_size, raw_size;
        uint8_t *packed=read_file(argv[i],&packed_size);
        uint8_t *raw=read_file(argv[i+1],&raw_size);
        assert(packed_size>=4 && raw_size<=sizeof(memory.vram));
        size_t decoded=(size_t)packed[1] | ((size_t)packed[2]<<8) | ((size_t)packed[3]<<16);
        assert(decoded==raw_size);
        FrlgNativeLzResource resource={packed,packed_size};
        assert(frlg_native_lz_resources_bind(&resource,1));
        memset(memory.vram,0xa5,sizeof(memory.vram));
        LZ77UnCompVram(packed,(void *)(uintptr_t)FRLG_GBA_VRAM_BASE);
        assert(!memcmp(raw,memory.vram,raw_size));
        if(raw_size<sizeof(memory.vram)) assert(memory.vram[raw_size]==0xa5);
        assert(frlg_native_lz_resources_bind(NULL,0));
        printf("Original startup asset matches: %zu compressed -> %zu decoded bytes\n",packed_size,raw_size);
        free(raw); free(packed);
    }
    return 0;
}
