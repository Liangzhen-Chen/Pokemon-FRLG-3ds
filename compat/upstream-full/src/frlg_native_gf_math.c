#include <math.h>

#include "global.h"
#include "util.h"
#include "gba/syscall.h"

static s16 sSineTable[256];
static bool8 sSineTableReady;

void StoreWordInTwoHalfwords(u16 *halves, u32 value)
{
    halves[0] = (u16)value;
    halves[1] = (u16)(value >> 16);
}

void LoadWordFromTwoHalfwords(u16 *halves, u32 *value)
{
    *value = (u32)halves[0] | ((u32)halves[1] << 16);
}

void ObjAffineSet(struct ObjAffineSrcData *src, void *dest, s32 count, s32 offset)
{
    const u8 *source = (const u8 *)src;
    u8 *target = dest;
    s32 i;

    if (count <= 0)
        return;

    if (!sSineTableReady)
    {
        for (i = 0; i < 256; i++)
            sSineTable[i] = (s16)(sin((double)i * 6.28318530717958647692 / 256.0) * 16384.0);
        sSineTableReady = TRUE;
    }

    for (i = 0; i < count; i++, source += 8)
    {
        const struct ObjAffineSrcData *item = (const struct ObjAffineSrcData *)source;
        u8 angle = item->rotation >> 8;
        s32 sine = sSineTable[angle];
        s32 cosine = sSineTable[(angle + 64) & 255];
        s32 pa = ((s32)item->xScale * cosine) >> 14;
        s32 pb = -(((s32)item->xScale * sine) >> 14);
        s32 pc = ((s32)item->yScale * sine) >> 14;
        s32 pd = ((s32)item->yScale * cosine) >> 14;

        *(s16 *)target = pa;
        target += offset;
        *(s16 *)target = pb;
        target += offset;
        *(s16 *)target = pc;
        target += offset;
        *(s16 *)target = pd;
        target += offset;
    }
}
