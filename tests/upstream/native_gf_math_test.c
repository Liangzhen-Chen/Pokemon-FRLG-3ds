#include <assert.h>

#include "global.h"
#include "util.h"
#include "gba/syscall.h"

static void check_word(u32 value)
{
    u16 words[4] = {0x1357, 0xffff, 0xffff, 0x2468};
    u32 restored = 0;

    StoreWordInTwoHalfwords(&words[1], value);
    assert(words[0] == 0x1357 && words[3] == 0x2468);
    assert(words[1] == (u16)value);
    assert(words[2] == (u16)(value >> 16));

    LoadWordFromTwoHalfwords(&words[1], &restored);
    assert(restored == value);
    assert(words[0] == 0x1357 && words[3] == 0x2468);
}

static void check_affine(struct ObjAffineSrcData src, s16 pa, s16 pb, s16 pc, s16 pd)
{
    s16 matrix[4] = {0x1234, 0x1234, 0x1234, 0x1234};

    ObjAffineSet(&src, matrix, 1, sizeof(s16));
    assert(matrix[0] == pa);
    assert(matrix[1] == pb);
    assert(matrix[2] == pc);
    assert(matrix[3] == pd);
}

static void check_affine_stride(void)
{
    struct AffineSlot
    {
        struct ObjAffineSrcData src;
        u16 padding;
    } slots[2] = {
        {{0x100, 0x100, 0}, 0x7f7f},
        {{0x100, 0x100, 0x4000}, 0x7f7f},
    };
    s16 adjacent[8] = {0};
    s16 spaced[16];
    int i;

    _Static_assert(sizeof(struct AffineSlot) == 8, "BIOS source stride");
    ObjAffineSet(&slots[0].src, adjacent, 2, sizeof(s16));
    assert(adjacent[0] == 256 && adjacent[1] == 0);
    assert(adjacent[2] == 0 && adjacent[3] == 256);
    assert(adjacent[4] == 0 && adjacent[5] == -256);
    assert(adjacent[6] == 256 && adjacent[7] == 0);

    for (i = 0; i < 16; i++)
        spaced[i] = 0x1234;
    ObjAffineSet(&slots[0].src, spaced, 1, 8);
    assert(spaced[0] == 256 && spaced[4] == 0);
    assert(spaced[8] == 0 && spaced[12] == 256);
    for (i = 0; i < 16; i++)
        if (i != 0 && i != 4 && i != 8 && i != 12)
            assert(spaced[i] == 0x1234);

    ObjAffineSet(&slots[0].src, spaced, 0, sizeof(s16));
    ObjAffineSet(&slots[0].src, spaced, -1, sizeof(s16));
    for (i = 0; i < 16; i++)
    {
        if (i == 0 || i == 12)
            assert(spaced[i] == 256);
        else if (i == 4 || i == 8)
            assert(spaced[i] == 0);
        else
            assert(spaced[i] == 0x1234);
    }
}

int main(void)
{
    check_word(0x00000000);
    check_word(0x89abcdef);
    check_word(0x80000000);
    check_affine((struct ObjAffineSrcData){0x100, 0x100, 0}, 256, 0, 0, 256);
    check_affine((struct ObjAffineSrcData){0x100, 0x100, 0x0100}, 255, -6, 6, 255);
    check_affine((struct ObjAffineSrcData){-0x100, 0x100, 0x0100}, -256, 7, 6, 255);
    check_affine((struct ObjAffineSrcData){0x100, 0x100, 0x01ff}, 255, -6, 6, 255);
    check_affine((struct ObjAffineSrcData){0x4000, 0x4000, 0x0200}, 16364, -803, 803, 16364);
    check_affine_stride();
    return 0;
}
