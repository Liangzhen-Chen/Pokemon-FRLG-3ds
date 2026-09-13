#ifndef FRLG_NATIVE_MAIN_H
#define FRLG_NATIVE_MAIN_H
#include "frlg_keypad.h"

/* Call once before a native game frame. Memory must be bound, InitKeys called,
 * and gSaveBlock2Ptr initialized to valid game storage. Uses original ReadKeys,
 * including its documented L=A repeat behavior. Does not run callbacks, reset
 * the game, advance play time, or simulate interrupts. Unknown bits are ignored. */
void frlg_native_main_read_keys(FrlgKeys held);
#endif
