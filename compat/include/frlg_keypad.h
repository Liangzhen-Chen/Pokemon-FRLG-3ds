#ifndef FRLG_KEYPAD_H
#define FRLG_KEYPAD_H

#include <stdint.h>

typedef uint16_t FrlgKeys;

enum FrlgKey {
    FRLG_KEY_A      = 1u << 0,
    FRLG_KEY_B      = 1u << 1,
    FRLG_KEY_SELECT = 1u << 2,
    FRLG_KEY_START  = 1u << 3,
    FRLG_KEY_RIGHT  = 1u << 4,
    FRLG_KEY_LEFT   = 1u << 5,
    FRLG_KEY_UP     = 1u << 6,
    FRLG_KEY_DOWN   = 1u << 7,
    FRLG_KEY_R      = 1u << 8,
    FRLG_KEY_L      = 1u << 9,
    FRLG_KEY_ALL    = 0x03ffu
};

enum {
    FRLG_KEY_REPEAT_START_FRAMES = 40,
    FRLG_KEY_REPEAT_CONTINUE_FRAMES = 5
};

typedef struct FrlgKeypad {
    FrlgKeys held;
    FrlgKeys pressed;
    FrlgKeys repeated;
    FrlgKeys pressed_or_repeated;
    FrlgKeys released;
    uint16_t repeat_counter;
} FrlgKeypad;

void frlg_keypad_reset(FrlgKeypad *keypad);
void frlg_keypad_update(FrlgKeypad *keypad, FrlgKeys current);

#endif
