#ifndef FRLG_UPSTREAM_NATIVE_GLOBAL_H
#define FRLG_UPSTREAM_NATIVE_GLOBAL_H

/* Narrow surface for GPU/DMA managers, not a full game global.h. */
#include "../../upstream-shim/include/global.h"
#include "frlg_native_io.h"
typedef uint8_t bool8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
#define FALSE 0
#define TRUE 1
#include "gba/io_reg.h"
struct BgAffineSrcData;
struct BgAffineDstData;
struct ObjAffineSrcData;
struct MultiBootParam;
#include "gba/syscall.h"
#include "gba/macro.h"

/* All register-address macros expand REG_BASE at their point of use. */
#undef REG_BASE
#define REG_BASE (frlg_native_io_base())
#define NELEMS(a) (sizeof(a)/sizeof((a)[0]))
#undef DmaCopy16
#undef DmaCopy32
#undef DmaFill16
#undef DmaFill32
#define DmaCopy16(ch,s,d,n) frlg_native_dma_copy(ch,(const void *)(uintptr_t)(s),(void *)(uintptr_t)(d),n,2)
#define DmaCopy32(ch,s,d,n) frlg_native_dma_copy(ch,(const void *)(uintptr_t)(s),(void *)(uintptr_t)(d),n,4)
#define DmaFill16(ch,v,d,n) frlg_native_dma_fill(ch,v,(void *)(uintptr_t)(d),n,2)
#define DmaFill32(ch,v,d,n) frlg_native_dma_fill(ch,v,(void *)(uintptr_t)(d),n,4)
#endif
