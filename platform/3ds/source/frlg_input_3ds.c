#include "frlg_input_3ds.h"

FrlgKeys frlg_input_3ds_map(u32 keys_held)
{
    FrlgKeys result = 0;

    if (keys_held & KEY_A)
        result |= FRLG_KEY_A;
    if (keys_held & KEY_B)
        result |= FRLG_KEY_B;
    if (keys_held & KEY_SELECT)
        result |= FRLG_KEY_SELECT;
    if (keys_held & KEY_START)
        result |= FRLG_KEY_START;
    if (keys_held & (KEY_DRIGHT | KEY_CPAD_RIGHT))
        result |= FRLG_KEY_RIGHT;
    if (keys_held & (KEY_DLEFT | KEY_CPAD_LEFT))
        result |= FRLG_KEY_LEFT;
    if (keys_held & (KEY_DUP | KEY_CPAD_UP))
        result |= FRLG_KEY_UP;
    if (keys_held & (KEY_DDOWN | KEY_CPAD_DOWN))
        result |= FRLG_KEY_DOWN;
    if (keys_held & KEY_R)
        result |= FRLG_KEY_R;
    if (keys_held & KEY_L)
        result |= FRLG_KEY_L;

    return result;
}
