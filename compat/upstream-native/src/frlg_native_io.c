#include "frlg_native_io.h"
#include "frlg_gba_lz77.h"
#include <stdlib.h>
#include <string.h>
static FrlgGbaMemory *bound_memory;
FrlgGbaMemory *frlg_native_memory(void)
{
    if(!bound_memory) abort();
    return bound_memory;
}
bool frlg_native_io_bind(FrlgGbaMemory *memory)
{
    if (!memory || ((uintptr_t)memory->io & 3)) return false;
    bound_memory = memory;
    return true;
}
uintptr_t frlg_native_io_base(void)
{
    if (!bound_memory) abort();
    return (uintptr_t)bound_memory->io;
}
static uint8_t *resolve(const void *pointer, size_t bytes, unsigned unit)
{
    uintptr_t address = (uintptr_t)pointer;
    if (!bound_memory || !address || address % unit || bytes > UINTPTR_MAX-address) return NULL;
    if (address >= FRLG_GBA_EWRAM_BASE && address < 0x08000000) {
        uint8_t *mapped;
        return frlg_gba_memory_map(bound_memory,(uint32_t)address,bytes,&mapped) ? mapped : NULL;
    }
    /* Other pointers are native game allocations; caller owns their size/lifetime. */
    return (uint8_t *)pointer;
}
bool frlg_native_copy(const void *source, void *dest, size_t bytes, unsigned unit)
{
    if ((unit != 2 && unit != 4) || bytes % unit) return false;
    if (!bytes) return true;
    uint8_t *src = resolve(source,bytes,unit), *dst = resolve(dest,bytes,unit);
    if (!src || !dst) return false;
    for (size_t offset=0;offset<bytes;offset+=unit) {
        uint8_t value[4];
        memcpy(value,src+offset,unit);
        memcpy(dst+offset,value,unit);
    }
    return true;
}
bool frlg_native_fill(uint32_t value, void *dest, size_t bytes, unsigned unit)
{
    if ((unit != 2 && unit != 4) || bytes % unit) return false;
    if (!bytes) return true;
    uint8_t *dst = resolve(dest,bytes,unit);
    if (!dst) return false;
    for (size_t offset=0;offset<bytes;offset+=unit)
        for (unsigned i=0;i<unit;i++) dst[offset+i]=(uint8_t)(value>>(8*i));
    return true;
}
void frlg_native_dma_copy(unsigned channel, const void *source, void *dest, size_t bytes, unsigned unit)
{
    if (channel > 3 || !frlg_native_copy(source,dest,bytes,unit)) abort();
}
void frlg_native_dma_fill(unsigned channel, uint32_t value, void *dest, size_t bytes, unsigned unit)
{
    if (channel > 3 || !frlg_native_fill(value,dest,bytes,unit)) abort();
}
bool frlg_native_cpu_set(const void *source, void *dest, uint32_t control, bool fast)
{
    size_t count = control & 0x1fffff;
    unsigned unit = (fast || (control & 0x04000000)) ? 4 : 2;
    bool fixed = (control & 0x01000000) != 0;
    if (!count) return true;
    if (fast) count = (count + 7) & ~(size_t)7;
    size_t bytes = count * unit;
    uint8_t *src = resolve(source,fixed ? unit : bytes,unit);
    uint8_t *dst = resolve(dest,bytes,unit);
    if (!src || !dst) return false;
    if (fixed) {
        uint32_t value = 0;
        for (unsigned i=0;i<unit;i++) value |= (uint32_t)src[i] << (8*i);
        return frlg_native_fill(value,dst,bytes,unit);
    }
    size_t block = fast ? 32 : unit;
    for (size_t offset=0;offset<bytes;offset+=block) {
        uint8_t temporary[32];
        memcpy(temporary,src+offset,block);
        memcpy(dst+offset,temporary,block);
    }
    return true;
}
void CpuSet(const void *source, void *dest, uint32_t control)
{
    if (!frlg_native_cpu_set(source,dest,control,false)) abort();
}
void CpuFastSet(const void *source, void *dest, uint32_t control)
{
    if (!frlg_native_cpu_set(source,dest,control,true)) abort();
}
static const FrlgNativeLzResource *lz_resources;
static size_t lz_resource_count;
bool frlg_native_lz_resources_bind(const FrlgNativeLzResource *resources, size_t count)
{
    if(!resources && count) return false;
    for(size_t i=0;i<count;i++) {
        if(!resources[i].data || resources[i].size<4) return false;
        for(size_t j=0;j<i;j++)
            if(resources[i].data==resources[j].data) return false;
    }
    lz_resources=resources;
    lz_resource_count=count;
    return true;
}
bool frlg_native_lz77(const void *source, size_t source_size, void *dest, bool vram)
{
    if(source_size<4) return false;
    uint8_t *src=resolve(source,source_size,1);
    if(!src || src[0]!=0x10) return false;
    size_t length=(size_t)src[1] | ((size_t)src[2]<<8) | ((size_t)src[3]<<16);
    if(!length) return false;
    uint8_t *dst=resolve(dest,length,vram ? 2 : 1);
    if(!dst) return false;
    return frlg_gba_lz77_decode(src,source_size,dst,length,vram);
}
static void decompress_registered(const void *source, void *dest, bool vram)
{
    for(size_t i=0;i<lz_resource_count;i++)
        if(source==lz_resources[i].data) {
            if(frlg_native_lz77(source,lz_resources[i].size,dest,vram)) return;
            break;
        }
    abort();
}
void LZ77UnCompWram(const void *source, void *dest)
{
    decompress_registered(source,dest,false);
}
void LZ77UnCompVram(const void *source, void *dest)
{
    decompress_registered(source,dest,true);
}
