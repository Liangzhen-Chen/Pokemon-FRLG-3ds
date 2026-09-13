#ifndef FRLG_UPSTREAM_FULL_GLOBAL_H
#define FRLG_UPSTREAM_FULL_GLOBAL_H
/* Keep the actual game structures and declarations. Local opt-in only. */
#include "../../../external/pokefirered/include/global.h"
#include "frlg_native_io.h"

/* Do not let upstream's macOS IDE conveniences fabricate strings or assets. */
#undef _
#undef __
#undef INCBIN
#undef INCBIN_U8
#undef INCBIN_U16
#undef INCBIN_U32
#undef INCBIN_S8
#undef INCBIN_S16
#undef INCBIN_S32

/* Native globals are normal allocations, not GBA linker-script sections. */
#undef EWRAM_DATA
#undef IWRAM_DATA
#undef COMMON_DATA
#define EWRAM_DATA
#define IWRAM_DATA
#define COMMON_DATA

#undef REG_BASE
#define REG_BASE (frlg_native_io_base())
#undef PLTT
#define PLTT ((uintptr_t)frlg_native_memory()->palette)
#undef DmaCopy16
#undef DmaCopy32
#undef DmaFill16
#undef DmaFill32
#define DmaCopy16(ch,s,d,n) frlg_native_dma_copy(ch,(const void *)(uintptr_t)(s),(void *)(uintptr_t)(d),n,2)
#define DmaCopy32(ch,s,d,n) frlg_native_dma_copy(ch,(const void *)(uintptr_t)(s),(void *)(uintptr_t)(d),n,4)
#define DmaFill16(ch,v,d,n) frlg_native_dma_fill(ch,v,(void *)(uintptr_t)(d),n,2)
#define DmaFill32(ch,v,d,n) frlg_native_dma_fill(ch,v,(void *)(uintptr_t)(d),n,4)
/* gflib.h includes malloc.h relative to upstream itself; load our binding
 * before that include so real game modules also use the mapped heap. */
#include "malloc.h"
#endif
