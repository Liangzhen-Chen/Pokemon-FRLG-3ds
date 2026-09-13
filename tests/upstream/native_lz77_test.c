#include <assert.h>
#include <string.h>
#include "frlg_gba_lz77.h"

int main(void)
{
    const uint8_t literal[]={0x10,4,0,0,0,1,2,3,4};
    const uint8_t repeat[]={0x10,8,0,0,0x20,'A','B',0x30,1};
    const uint8_t distance_one[]={0x10,4,0,0,0x40,'A',0,0};
    const uint8_t bad_distance[]={0x10,4,0,0,0x80,0,0};
    const uint8_t odd[]={0x10,3,0,0,0,1,2,3};
    const uint8_t oversized_run[]={0x10,4,0,0,0x40,'A',0xf0,0};
    uint8_t output[32];
    memset(output,0xa5,sizeof(output));
    assert(frlg_gba_lz77_decode(literal,sizeof(literal),output,sizeof(output),false));
    assert(!memcmp(output,literal+5,4) && output[4]==0xa5);
    assert(frlg_gba_lz77_decode(repeat,sizeof(repeat),output,sizeof(output),true));
    assert(!memcmp(output,"ABABABAB",8) && output[8]==0xa5);
    assert(frlg_gba_lz77_decode(distance_one,sizeof(distance_one),output,sizeof(output),false));
    assert(!memcmp(output,"AAAA",4));
    assert(frlg_gba_lz77_decode(odd,sizeof(odd),output,sizeof(output),false));
    memset(output,0xa5,sizeof(output));
    for(size_t size=0;size<sizeof(literal);size++)
        assert(!frlg_gba_lz77_decode(literal,size,output,sizeof(output),false));
    for(size_t size=0;size<sizeof(repeat);size++)
        assert(!frlg_gba_lz77_decode(repeat,size,output,sizeof(output),false));
    assert(!frlg_gba_lz77_decode(literal,sizeof(literal),output,3,false));
    assert(!frlg_gba_lz77_decode(distance_one,sizeof(distance_one),output,sizeof(output),true));
    assert(!frlg_gba_lz77_decode(bad_distance,sizeof(bad_distance),output,sizeof(output),false));
    assert(!frlg_gba_lz77_decode(odd,sizeof(odd),output,sizeof(output),true));
    assert(!frlg_gba_lz77_decode(oversized_run,sizeof(oversized_run),output,sizeof(output),false));
    const uint8_t wrong_type[]={0x11,4,0,0,0,1,2,3,4};
    assert(!frlg_gba_lz77_decode(wrong_type,sizeof(wrong_type),output,sizeof(output),false));
    assert(!frlg_gba_lz77_decode(NULL,4,output,sizeof(output),false));
    for(unsigned i=0;i<sizeof(output);i++) assert(output[i]==0xa5);
    memcpy(output,literal,sizeof(literal));
    assert(!frlg_gba_lz77_decode(output,sizeof(literal),output+4,20,false));
    assert(!memcmp(output,literal,sizeof(literal)));
    return 0;
}
