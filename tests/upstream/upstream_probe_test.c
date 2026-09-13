#include <assert.h>
#include <stdint.h>

#include "frlg_upstream_probe.h"

static uint16_t expected_first_random(uint16_t seed)
{
    const uint32_t next = UINT32_C(1103515245) * seed + UINT32_C(24691);
    return (uint16_t)(next >> 16);
}

int main(void)
{
    _Static_assert(sizeof(uint8_t) == 1, "8-bit type required");
    _Static_assert(sizeof(uint16_t) == 2, "16-bit type required");
    _Static_assert(sizeof(uint32_t) == 4, "32-bit type required");
    _Static_assert(sizeof(int64_t) == 8, "64-bit type required");

    frlg_upstream_probe_seed(0);
    assert(frlg_upstream_probe_next_random() == expected_first_random(0));
    frlg_upstream_probe_seed(1);
    assert(frlg_upstream_probe_next_random() == UINT16_C(0x41c6));
    frlg_upstream_probe_seed(UINT16_C(0xffff));
    assert(frlg_upstream_probe_next_random() == expected_first_random(UINT16_C(0xffff)));

    frlg_upstream_probe_seed(UINT16_C(0x1234));
    assert(frlg_upstream_probe_next_random() == expected_first_random(UINT16_C(0x1234)));

    assert(frlg_upstream_probe_q8_mul(256, 128) == 128);
    assert(frlg_upstream_probe_q8_mul(-256, 128) == -128);
    assert(frlg_upstream_probe_q8_div(128, 256) == 128);
    assert(frlg_upstream_probe_q8_div(-128, 256) == -128);
    assert(frlg_upstream_probe_q8_div(1, 0) == 0);

    return 0;
}
