#ifndef FRLG_GBA_MODE0_H
#define FRLG_GBA_MODE0_H

#include "frlg_gba_mode3.h"

/* Bounded Mode 0 text BGs, 4bpp 1D non-affine OBJs, WIN0/WIN1/OBJ windows.
 * Unsupported configurations leave output intact. */
bool frlg_gba_mode0_render(const FrlgGbaMemory *memory,
                          const FrlgGbaDisplaySnapshot *display,
                          FrlgRgb8 *output, size_t output_pixels);

/* Render the published HBlank BLDY value for each visible scanline. */
bool frlg_gba_mode0_render_with_bldy(const FrlgGbaMemory *memory,
                                    const FrlgGbaDisplaySnapshot *display,
                                    FrlgRgb8 *output, size_t output_pixels,
                                    const uint16_t bldy_by_line[FRLG_GBA_SCREEN_HEIGHT]);

#endif
