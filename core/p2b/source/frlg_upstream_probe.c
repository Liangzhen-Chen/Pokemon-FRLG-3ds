#include "frlg_upstream_probe.h"

#include "global.h"
#include "math_util.h"
#include "random.h"

void frlg_upstream_probe_seed(uint16_t seed)
{
    SeedRng((u16)seed);
}

uint16_t frlg_upstream_probe_next_random(void)
{
    return (uint16_t)Random();
}

int16_t frlg_upstream_probe_q8_mul(int16_t x, int16_t y)
{
    return (int16_t)Q_8_8_mul((s16)x, (s16)y);
}

int16_t frlg_upstream_probe_q8_div(int16_t x, int16_t y)
{
    return (int16_t)Q_8_8_div((s16)x, (s16)y);
}
