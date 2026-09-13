#ifndef FRLG_NATIVE_SCANLINE_H
#define FRLG_NATIVE_SCANLINE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum FrlgNativeScanlineStatus
{
    FRLG_NATIVE_SCANLINE_OK = 0,
    FRLG_NATIVE_SCANLINE_UNSUPPORTED_DEST,
    FRLG_NATIVE_SCANLINE_UNSUPPORTED_CONTROL,
    FRLG_NATIVE_SCANLINE_UNSUPPORTED_STATE,
    FRLG_NATIVE_SCANLINE_INVALID_ARGUMENT
} FrlgNativeScanlineStatus;

/* A frame is active only after the original VBlank transfer publishes it. */
void frlg_native_scanline_begin_frame(void);
FrlgNativeScanlineStatus frlg_native_scanline_status(void);
FrlgNativeScanlineStatus frlg_native_scanline_copy_frame(uint16_t out_bldy[160], bool *active);

#endif
