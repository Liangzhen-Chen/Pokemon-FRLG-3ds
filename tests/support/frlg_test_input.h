#ifndef FRLG_TEST_INPUT_H
#define FRLG_TEST_INPUT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t session;
    uint32_t sequence;
    uint16_t keys, remaining;
} FrlgTestInput;

bool frlg_test_input_accept(FrlgTestInput *state, const uint8_t *packet, size_t size);
uint16_t frlg_test_input_tick(FrlgTestInput *state);
#endif
