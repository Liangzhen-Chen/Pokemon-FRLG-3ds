#include <assert.h>
#include <string.h>
#include "global.h"
#include "gflib.h"

extern bool32 CheckHeap(void);
extern bool32 CheckMemBlock(void *);
static unsigned failures;

/* Observe the original allocator's exhaustion assertion in this test only. */
void AGBAssert(const char *file, int line, const char *expression, int stop)
{
    assert(file && expression && line > 0 && stop);
    failures++;
}

int main(void)
{
    _Alignas(8) static FrlgGbaMemory memory;
    assert(frlg_native_io_bind(&memory));
    assert(gHeap == memory.ewram);
    memset(memory.ewram, 0xA5, sizeof(memory.ewram));
    InitHeap(gHeap, HEAP_SIZE);
    assert(CheckHeap());
    /* Host pointers need 8-byte alignment. This test does not establish
       correctness of odd-sized allocations under the 32-bit target ABI. */
    u8 *a = Alloc(32), *b = AllocZeroed(64), *c = Alloc(128);
    assert(a && b && c && a != b && b != c);
    assert(CheckMemBlock(a) && CheckMemBlock(b) && CheckMemBlock(c));
    for (unsigned i = 0; i < 64; i++) assert(b[i] == 0);
    memset(a, 0x12, 32);
    memset(c, 0x34, 128);
    Free(b);
    assert(CheckHeap());
    assert(AllocZeroed(64) == b);
    for (unsigned i = 0; i < 32; i++) assert(a[i] == 0x12);
    for (unsigned i = 0; i < 128; i++) assert(c[i] == 0x34);
    Free(a);
    Free(c);
    Free(b);
    assert(CheckHeap());
    a = Alloc(HEAP_SIZE - 64);
    assert(a && CheckHeap());
    assert(Alloc(128) == NULL && failures == 1);
    Free(a);
    InitHeap(gHeap, HEAP_SIZE);
    assert(CheckHeap() && Alloc(32) == a);
    for (unsigned i = HEAP_SIZE; i < sizeof(memory.ewram); i++)
        assert(memory.ewram[i] == 0xA5);
    return 0;
}
