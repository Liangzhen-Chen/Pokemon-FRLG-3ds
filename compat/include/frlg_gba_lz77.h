#ifndef FRLG_GBA_LZ77_H
#define FRLG_GBA_LZ77_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Bounded 0x10 stream. Source/destination must be disjoint and remain stable.
 * Failure leaves destination unchanged. VRAM requires even decoded size and
 * rejects references to the pending, not-yet-written halfword byte. */
bool frlg_gba_lz77_decode(const uint8_t *source, size_t source_size,
                         uint8_t *dest, size_t dest_size, bool vram);
#endif
