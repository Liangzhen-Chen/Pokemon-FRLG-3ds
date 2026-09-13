#include <assert.h>
#include "frlg_native_io.h"

int main(void)
{
    _Alignas(4) static FrlgGbaMemory first, second;
    assert(!frlg_native_io_bind(NULL));
    assert(frlg_native_io_bind(&first));
    assert(frlg_native_memory()==&first);
    assert(frlg_native_io_base() == (uintptr_t)first.io);
    assert((frlg_native_io_base() & 3) == 0);
    assert(!frlg_native_io_bind(NULL));
    assert(frlg_native_io_base() == (uintptr_t)first.io);
    assert(frlg_native_io_bind(&second));
    assert(frlg_native_memory()==&second);
    assert(frlg_native_io_base() == (uintptr_t)second.io);
    return 0;
}
