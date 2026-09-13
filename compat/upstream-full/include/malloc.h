#ifndef FRLG_UPSTREAM_FULL_MALLOC_H
#define FRLG_UPSTREAM_FULL_MALLOC_H
#include "../../../external/pokefirered/include/malloc.h"

/* ld_script_modern.ld reserves the first 0x1C000 bytes of EWRAM for
 * gHeap. Share the mapped bytes rather than allocate a second heap. */
_Static_assert(HEAP_SIZE <= FRLG_GBA_EWRAM_SIZE, "heap exceeds EWRAM");
#define gHeap (frlg_native_memory()->ewram)
#endif
