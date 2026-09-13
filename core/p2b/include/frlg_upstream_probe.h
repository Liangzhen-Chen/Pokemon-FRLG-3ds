#ifndef FRLG_UPSTREAM_PROBE_H
#define FRLG_UPSTREAM_PROBE_H

#include <stdint.h>

void frlg_upstream_probe_seed(uint16_t seed);
uint16_t frlg_upstream_probe_next_random(void);
int16_t frlg_upstream_probe_q8_mul(int16_t x, int16_t y);
int16_t frlg_upstream_probe_q8_div(int16_t x, int16_t y);

#endif
