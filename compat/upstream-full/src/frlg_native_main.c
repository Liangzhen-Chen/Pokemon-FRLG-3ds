/* Compile instead of upstream main.c, not alongside it. Keep the original
 * private reader and game state in their translation unit without editing
 * the locked source. AgbMain remains GBA-only and must not be executed. */
#include "../../../external/pokefirered/src/main.c"
#include "frlg_native_main.h"

_Static_assert(FRLG_KEY_ALL == KEYS_MASK, "native key mask differs from GBA");

void frlg_native_main_read_keys(FrlgKeys held)
{
    REG_KEYINPUT = (held & KEYS_MASK) ^ KEYS_MASK;
    ReadKeys();
}
