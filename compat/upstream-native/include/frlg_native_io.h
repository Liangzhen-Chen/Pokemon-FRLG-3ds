#ifndef FRLG_NATIVE_IO_H
#define FRLG_NATIVE_IO_H
#include "frlg_gba_memory.h"

/* One instance per process, little-endian host, memory aligned to at least 4 bytes.
 * Bind before calling upstream GPU routines; unbound access aborts, never uses GBA IO.
 * Raw upstream APIs require valid, aligned register offsets. */
bool frlg_native_io_bind(FrlgGbaMemory *memory);
uintptr_t frlg_native_io_base(void);
/* Direct native access for full-header register/palette aliases; aborts unbound.
 * Returned storage is owned by the binder; rebinding changes future lookups. */
FrlgGbaMemory *frlg_native_memory(void);
/* Synchronous byte-count API, unit 2 or 4; zero bytes is a no-op.
 * GBA addresses in [0x02000000,0x08000000) require a complete mapped range.
 * Other pointers must be valid native allocations owned by the caller.
 * Copy proceeds forward by unit (not memmove); no hardware DMA timing.
 * Invalid ranges fail before this call writes; wrappers abort on failure.
 * Multi-chunk upstream requests are not transactional across calls. */
bool frlg_native_copy(const void *source, void *dest, size_t bytes, unsigned unit);
bool frlg_native_fill(uint32_t value, void *dest, size_t bytes, unsigned unit);
void frlg_native_dma_copy(unsigned channel, const void *source, void *dest, size_t bytes, unsigned unit);
void frlg_native_dma_fill(unsigned channel, uint32_t value, void *dest, size_t bytes, unsigned unit);
/* FRLG CpuSet control: low 21-bit unit count, bit 24 fixed source, bit 26 word.
 * Fast mode is always word-sized and rounds up to eight-word blocks, loading
 * each block before writing it. Requires aligned, valid pointers like DMA.
 * Zero count never touches memory, including fixed-source calls. Other bits
 * are ignored. No BIOS timing, misalignment quirks or CPU-register clobbers.
 * Checked API returns false; ABI-compatible entrypoints below abort on error. */
bool frlg_native_cpu_set(const void *source, void *dest, uint32_t control, bool fast);
void CpuSet(const void *source, void *dest, uint32_t control);
void CpuFastSet(const void *source, void *dest, uint32_t control);
typedef struct FrlgNativeLzResource {
    const uint8_t *data;
    size_t size;
} FrlgNativeLzResource;
/* Immutable table/data must outlive use. Bind NULL,0 to clear. Original BIOS
 * ABI has no compressed length, so it accepts only explicitly registered data.
 * Direct checked calls accept a source size; native destination capacity is
 * still the caller's responsibility, mapped GBA destinations are checked. */
bool frlg_native_lz_resources_bind(const FrlgNativeLzResource *resources, size_t count);
bool frlg_native_lz77(const void *source, size_t source_size, void *dest, bool vram);
void LZ77UnCompWram(const void *source, void *dest);
void LZ77UnCompVram(const void *source, void *dest);
#endif
