#include <assert.h>
#include "global.h"
#include "main.h"
#include "frlg_native_main.h"

/* Fixture for the real reader's save-option dependency, not a save backend. */
static struct SaveBlock2 save;
struct SaveBlock2 *gSaveBlock2Ptr = &save;

static void callback(void) {}

int main(void)
{
    _Alignas(8) static FrlgGbaMemory memory;
    assert(frlg_native_io_bind(&memory));
    InitKeys();
    frlg_native_main_read_keys(0);
    assert(REG_KEYINPUT == KEYS_MASK && gMain.newKeys == 0);
    frlg_native_main_read_keys(0xFC00);
    assert(REG_KEYINPUT == KEYS_MASK && gMain.heldKeysRaw == 0);
    frlg_native_main_read_keys(A_BUTTON);
    assert(gMain.newKeysRaw == A_BUTTON && gMain.heldKeys == A_BUTTON);
    assert(gMain.newAndRepeatedKeys == A_BUTTON && gMain.keyRepeatCounter == 40);
    for (unsigned i=0; i<39; i++) {
        frlg_native_main_read_keys(A_BUTTON);
        assert(gMain.newKeys == 0 && gMain.newAndRepeatedKeys == 0);
    }
    frlg_native_main_read_keys(A_BUTTON);
    assert(gMain.newAndRepeatedKeys == A_BUTTON && gMain.keyRepeatCounter == 5);
    for (unsigned i=0; i<4; i++) {
        frlg_native_main_read_keys(A_BUTTON);
        assert(!gMain.newAndRepeatedKeys);
    }
    frlg_native_main_read_keys(A_BUTTON);
    assert(gMain.newAndRepeatedKeys == A_BUTTON);
    frlg_native_main_read_keys(0);
    assert(!gMain.heldKeysRaw && !gMain.newKeys);
    gMain.watchedKeysMask = B_BUTTON;
    frlg_native_main_read_keys(B_BUTTON | DPAD_RIGHT);
    assert(gMain.newKeys == (B_BUTTON | DPAD_RIGHT) && gMain.watchedKeysPressed);
    frlg_native_main_read_keys(0);
    save.optionsButtonMode = OPTIONS_BUTTON_MODE_L_EQUALS_A;
    frlg_native_main_read_keys(L_BUTTON);
    assert(gMain.newKeysRaw == L_BUTTON && gMain.heldKeysRaw == L_BUTTON);
    assert(gMain.newKeys == (L_BUTTON | A_BUTTON));
    assert(gMain.heldKeys == (L_BUTTON | A_BUTTON));
    /* Preserve the documented original L=A repeat bug; do not silently fix it. */
    for (unsigned i=0; i<45; i++) frlg_native_main_read_keys(L_BUTTON);
    assert(!gMain.newAndRepeatedKeys && gMain.keyRepeatCounter == 40);
    gMain.state = 17;
    SetMainCallback2(callback);
    assert(gMain.callback2 == callback && gMain.state == 0);
    return 0;
}
