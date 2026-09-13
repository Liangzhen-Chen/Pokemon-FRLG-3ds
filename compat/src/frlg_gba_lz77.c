#include "frlg_gba_lz77.h"

static bool walk(const uint8_t *source, size_t source_size, uint8_t *dest,
                 size_t length, bool vram, bool write)
{
    size_t input=4, output=0;
    while(output<length) {
        if(input>=source_size) return false;
        uint8_t flags=source[input++];
        for(unsigned bit=0;bit<8 && output<length;bit++) {
            if(flags & (0x80u>>bit)) {
                if(source_size-input<2) return false;
                size_t count=(source[input]>>4)+3;
                size_t distance=(((size_t)source[input]&15)<<8)+source[input+1]+1;
                input+=2;
                if(distance>output || count>length-output) return false;
                for(size_t i=0;i<count;i++,output++) {
                    if(vram && output-distance >= (output & ~(size_t)1)) return false;
                    if(write) dest[output]=dest[output-distance];
                }
            } else {
                if(input>=source_size) return false;
                if(write) dest[output]=source[input];
                input++; output++;
            }
        }
    }
    return true;
}

bool frlg_gba_lz77_decode(const uint8_t *source, size_t source_size,
                         uint8_t *dest, size_t dest_size, bool vram)
{
    if(!source || !dest || source_size<4 || source[0]!=0x10) return false;
    size_t length=(size_t)source[1] | ((size_t)source[2]<<8) | ((size_t)source[3]<<16);
    if(!length || length>dest_size || (vram && (length&1))) return false;
    uintptr_t src=(uintptr_t)source, dst=(uintptr_t)dest;
    if(source_size>UINTPTR_MAX-src || length>UINTPTR_MAX-dst) return false;
    if(src<dst+length && dst<src+source_size) return false;
    if(!walk(source,source_size,dest,length,vram,false)) return false;
    return walk(source,source_size,dest,length,vram,true);
}
