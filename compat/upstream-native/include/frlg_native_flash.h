#ifndef FRLG_NATIVE_FLASH_H
#define FRLG_NATIVE_FLASH_H

#include <stdint.h>

typedef enum {
    FRLG_NATIVE_FLASH_OK = 0,
    FRLG_NATIVE_FLASH_NO_MEDIA,
    FRLG_NATIVE_FLASH_BAD_ID,
    FRLG_NATIVE_FLASH_BAD_SIZE,
    FRLG_NATIVE_FLASH_IO,
    FRLG_NATIVE_FLASH_BAD_FORMAT,
    FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED
} FrlgNativeFlashResult;

/* Explicit M1 erased-media test only. Bind native IO before calling. */
FrlgNativeFlashResult frlg_native_flash_start_erased(const char *path, uint16_t device_id);
FrlgNativeFlashResult frlg_native_flash_last_result(void);

#endif
