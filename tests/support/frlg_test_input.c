#include "frlg_test_input.h"
#include <string.h>

static uint64_t little(const uint8_t *p, unsigned n)
{
    uint64_t value = 0;
    for (unsigned i = 0; i < n; i++) value |= (uint64_t)p[i] << (8 * i);
    return value;
}

bool frlg_test_input_accept(FrlgTestInput *state, const uint8_t *packet, size_t size)
{
    if (!state || !packet || size != 24 || memcmp(packet,"FRI1",4)) return false;
    uint64_t session = little(packet + 4,8);
    uint32_t sequence = (uint32_t)little(packet + 12,4);
    uint16_t keys = (uint16_t)little(packet + 16,2);
    uint16_t frames = (uint16_t)little(packet + 18,2);
    if (!session || session != state->session || sequence <= state->sequence ||
        (keys & ~0x03ffu) || frames > 120 || ((keys == 0) != (frames == 0)) ||
        little(packet + 20,4)) return false;
    state->sequence = sequence;
    state->keys = keys;
    state->remaining = frames;
    return true;
}
uint16_t frlg_test_input_tick(FrlgTestInput *state)
{
    if (!state->remaining) return 0;
    state->remaining--;
    return state->keys;
}
