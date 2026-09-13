#ifndef FRLG_GBA_MODE0_H
#define FRLG_GBA_MODE0_H

#include "frlg_gba_mode3.h"

/* Bounded 4bpp/8bpp text backgrounds. Unsupported configurations leave output intact. */
bool frlg_gba_mode0_render(const FrlgGbaMemory *memory,
                          const FrlgGbaDisplaySnapshot *display,
                          FrlgRgb8 *output, size_t output_pixels);

#endif
