#include <assert.h>

#include "frlg_keypad.h"

static void test_key_layout(void)
{
    assert(FRLG_KEY_A == 0x0001);
    assert(FRLG_KEY_B == 0x0002);
    assert(FRLG_KEY_SELECT == 0x0004);
    assert(FRLG_KEY_START == 0x0008);
    assert(FRLG_KEY_RIGHT == 0x0010);
    assert(FRLG_KEY_LEFT == 0x0020);
    assert(FRLG_KEY_UP == 0x0040);
    assert(FRLG_KEY_DOWN == 0x0080);
    assert(FRLG_KEY_R == 0x0100);
    assert(FRLG_KEY_L == 0x0200);
    assert(FRLG_KEY_ALL == 0x03ff);
}

static void test_edges_and_repeat(void)
{
    FrlgKeypad keypad;
    frlg_keypad_reset(&keypad);

    frlg_keypad_update(&keypad, FRLG_KEY_A);
    assert(keypad.pressed == FRLG_KEY_A);
    assert(keypad.held == FRLG_KEY_A);
    assert(keypad.pressed_or_repeated == FRLG_KEY_A);
    assert(keypad.repeated == 0);
    assert(keypad.repeat_counter == FRLG_KEY_REPEAT_START_FRAMES);

    for (int frame = 1; frame < FRLG_KEY_REPEAT_START_FRAMES; frame++)
    {
        frlg_keypad_update(&keypad, FRLG_KEY_A);
        assert(keypad.pressed == 0);
        assert(keypad.repeated == 0);
    }

    frlg_keypad_update(&keypad, FRLG_KEY_A);
    assert(keypad.repeated == FRLG_KEY_A);
    assert(keypad.pressed_or_repeated == FRLG_KEY_A);

    for (int frame = 1; frame < FRLG_KEY_REPEAT_CONTINUE_FRAMES; frame++)
    {
        frlg_keypad_update(&keypad, FRLG_KEY_A);
        assert(keypad.repeated == 0);
    }

    frlg_keypad_update(&keypad, FRLG_KEY_A);
    assert(keypad.repeated == FRLG_KEY_A);

    frlg_keypad_update(&keypad, FRLG_KEY_A | FRLG_KEY_B);
    assert(keypad.pressed == FRLG_KEY_B);
    assert(keypad.released == 0);
    assert(keypad.repeat_counter == FRLG_KEY_REPEAT_START_FRAMES);

    frlg_keypad_update(&keypad, FRLG_KEY_B);
    assert(keypad.pressed == 0);
    assert(keypad.released == FRLG_KEY_A);

    frlg_keypad_update(&keypad, 0xffff);
    assert(keypad.held == FRLG_KEY_ALL);
}

void run_keypad_tests(void)
{
    test_key_layout();
    test_edges_and_repeat();
}
