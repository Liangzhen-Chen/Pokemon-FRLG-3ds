#include "frlg_runtime.h"

static int16_t clamp_position(int16_t value, int16_t maximum)
{
    if (value < 0)
        return 0;
    if (value > maximum)
        return maximum;
    return value;
}

void frlg_runtime_init(FrlgRuntime *runtime)
{
    runtime->frame = 0;
    runtime->phase = FRLG_PROBE_BOOT;
    runtime->probe_x = 120;
    runtime->probe_y = 80;
    runtime->last_pressed = 0;
}

void frlg_runtime_step(FrlgRuntime *runtime, const FrlgKeypad *keypad)
{
    const FrlgKeys movement = keypad->pressed_or_repeated;

    runtime->last_pressed = keypad->pressed;

    if (keypad->pressed & FRLG_KEY_A)
        runtime->phase = FRLG_PROBE_READY;
    if (keypad->pressed & FRLG_KEY_B)
        runtime->phase = FRLG_PROBE_BOOT;

    if (movement & FRLG_KEY_LEFT)
        runtime->probe_x--;
    if (movement & FRLG_KEY_RIGHT)
        runtime->probe_x++;
    if (movement & FRLG_KEY_UP)
        runtime->probe_y--;
    if (movement & FRLG_KEY_DOWN)
        runtime->probe_y++;

    runtime->probe_x = clamp_position(runtime->probe_x, 239);
    runtime->probe_y = clamp_position(runtime->probe_y, 159);
    runtime->frame++;
}

FrlgSnapshot frlg_runtime_snapshot(const FrlgRuntime *runtime)
{
    const FrlgSnapshot snapshot = {
        .frame = runtime->frame,
        .phase = runtime->phase,
        .probe_x = runtime->probe_x,
        .probe_y = runtime->probe_y,
        .last_pressed = runtime->last_pressed,
    };

    return snapshot;
}

const char *frlg_probe_phase_name(FrlgProbePhase phase)
{
    return phase == FRLG_PROBE_READY ? "READY" : "BOOT";
}
