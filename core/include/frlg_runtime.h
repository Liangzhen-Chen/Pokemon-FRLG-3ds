#ifndef FRLG_RUNTIME_H
#define FRLG_RUNTIME_H

#include <stdint.h>

#include "frlg_keypad.h"

typedef enum FrlgProbePhase {
    FRLG_PROBE_BOOT = 0,
    FRLG_PROBE_READY = 1
} FrlgProbePhase;

typedef struct FrlgRuntime {
    uint64_t frame;
    FrlgProbePhase phase;
    int16_t probe_x;
    int16_t probe_y;
    FrlgKeys last_pressed;
} FrlgRuntime;

typedef struct FrlgSnapshot {
    uint64_t frame;
    FrlgProbePhase phase;
    int16_t probe_x;
    int16_t probe_y;
    FrlgKeys last_pressed;
} FrlgSnapshot;

void frlg_runtime_init(FrlgRuntime *runtime);
void frlg_runtime_step(FrlgRuntime *runtime, const FrlgKeypad *keypad);
FrlgSnapshot frlg_runtime_snapshot(const FrlgRuntime *runtime);
const char *frlg_probe_phase_name(FrlgProbePhase phase);

#endif
