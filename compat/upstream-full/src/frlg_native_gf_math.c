#include "global.h"
#include "util.h"

void StoreWordInTwoHalfwords(u16 *halves, u32 value)
{
    halves[0] = (u16)value;
    halves[1] = (u16)(value >> 16);
}

void LoadWordFromTwoHalfwords(u16 *halves, u32 *value)
{
    *value = (u32)halves[0] | ((u32)halves[1] << 16);
}
