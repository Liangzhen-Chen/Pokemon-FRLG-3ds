#include "frlg_keypad.h"

void frlg_keypad_reset(FrlgKeypad *keypad)
{
    keypad->held = 0;
    keypad->pressed = 0;
    keypad->repeated = 0;
    keypad->pressed_or_repeated = 0;
    keypad->released = 0;
    keypad->repeat_counter = FRLG_KEY_REPEAT_START_FRAMES;
}

void frlg_keypad_update(FrlgKeypad *keypad, FrlgKeys current)
{
    const FrlgKeys previous = keypad->held;

    current &= FRLG_KEY_ALL;
    keypad->pressed = current & (FrlgKeys)~previous;
    keypad->released = previous & (FrlgKeys)~current;
    keypad->repeated = 0;
    keypad->pressed_or_repeated = keypad->pressed;

    if (current != 0 && current == previous)
    {
        if (keypad->repeat_counter > 0)
            keypad->repeat_counter--;

        if (keypad->repeat_counter == 0)
        {
            keypad->repeated = current;
            keypad->pressed_or_repeated |= current;
            keypad->repeat_counter = FRLG_KEY_REPEAT_CONTINUE_FRAMES;
        }
    }
    else
    {
        keypad->repeat_counter = FRLG_KEY_REPEAT_START_FRAMES;
    }

    keypad->held = current;
}
