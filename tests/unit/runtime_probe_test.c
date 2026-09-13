#include <assert.h>

#include "frlg_runtime.h"

void run_keypad_tests(void);
void run_gba_memory_tests(void);
void run_gba_io_dma_tests(void);
void run_gba_mode3_tests(void);
void run_gba_mode0_tests(void);

int main(void)
{
    run_keypad_tests();
    run_gba_memory_tests();
    run_gba_io_dma_tests();
    run_gba_mode3_tests();
    run_gba_mode0_tests();

    FrlgKeypad keypad;
    FrlgRuntime runtime;

    frlg_keypad_reset(&keypad);
    frlg_runtime_init(&runtime);

    FrlgSnapshot snapshot = frlg_runtime_snapshot(&runtime);
    assert(snapshot.frame == 0);
    assert(snapshot.phase == FRLG_PROBE_BOOT);
    assert(snapshot.probe_x == 120);
    assert(snapshot.probe_y == 80);

    frlg_keypad_update(&keypad, FRLG_KEY_A | FRLG_KEY_RIGHT);
    frlg_runtime_step(&runtime, &keypad);
    snapshot = frlg_runtime_snapshot(&runtime);
    assert(snapshot.frame == 1);
    assert(snapshot.phase == FRLG_PROBE_READY);
    assert(snapshot.probe_x == 121);
    assert(snapshot.last_pressed == (FRLG_KEY_A | FRLG_KEY_RIGHT));

    frlg_keypad_update(&keypad, FRLG_KEY_RIGHT);
    frlg_runtime_step(&runtime, &keypad);
    for (int frame = 1; frame < FRLG_KEY_REPEAT_START_FRAMES; frame++)
    {
        frlg_keypad_update(&keypad, FRLG_KEY_RIGHT);
        frlg_runtime_step(&runtime, &keypad);
    }
    snapshot = frlg_runtime_snapshot(&runtime);
    assert(snapshot.probe_x == 121);

    frlg_keypad_update(&keypad, FRLG_KEY_RIGHT);
    frlg_runtime_step(&runtime, &keypad);
    snapshot = frlg_runtime_snapshot(&runtime);
    assert(snapshot.probe_x == 122);

    frlg_keypad_update(&keypad, FRLG_KEY_B);
    frlg_runtime_step(&runtime, &keypad);
    snapshot = frlg_runtime_snapshot(&runtime);
    assert(snapshot.phase == FRLG_PROBE_BOOT);
    assert(snapshot.last_pressed == FRLG_KEY_B);
    assert(frlg_probe_phase_name(snapshot.phase)[0] == 'B');

    return 0;
}
