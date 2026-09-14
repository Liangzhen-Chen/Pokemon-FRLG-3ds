#ifndef FRLG_NATIVE_MAIN_H
#define FRLG_NATIVE_MAIN_H
#include "frlg_keypad.h"

typedef enum {
    FRLG_NATIVE_TITLE_EXIT_OK,
    FRLG_NATIVE_TITLE_EXIT_MAIN_MENU,
    FRLG_NATIVE_TITLE_EXIT_SAVE_CLEAR,
    FRLG_NATIVE_TITLE_EXIT_BERRY_FIX,
} FrlgNativeTitleExitStatus;

FrlgNativeTitleExitStatus frlg_native_title_exit_status(void);

/* Call once before a native game frame. Memory must be bound, InitKeys called,
 * and gSaveBlock2Ptr initialized to valid game storage. Uses original ReadKeys,
 * including its documented L=A repeat behavior. Does not run callbacks, reset
 * the game, advance play time, or simulate interrupts. Unknown bits are ignored. */
void frlg_native_main_read_keys(FrlgKeys held);
/* Native boot keeps the original callback and frame work in main.c while the
 * 3DS owns scheduling. Bind GBA memory and services before calling init.
 * flash_path must name a complete erased 128-KiB read-only test image. */
bool frlg_native_main_init(const char *flash_path);
/* False stops the frame on an unsupported operation or a sticky audio error. */
bool frlg_native_main_step(FrlgKeys held);
#endif
