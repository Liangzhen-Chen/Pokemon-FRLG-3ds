#include <assert.h>

#include "global.h"
#include "util.h"

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

int main(void)
{
    check_word(0x00000000);
    check_word(0x89abcdef);
    check_word(0x80000000);
    return 0;
}
