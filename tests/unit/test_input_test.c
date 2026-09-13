#include <assert.h>
#include <string.h>
#include "frlg_test_input.h"

int main(void)
{
    FrlgTestInput state = {.session = 7};
    uint8_t p[24] = {'F','R','I','1',7,0,0,0,0,0,0,0,1,0,0,0,1,0,2,0,0,0,0,0};
    assert(frlg_test_input_accept(&state, p, sizeof(p)));
    assert(state.sequence == 1);
    assert(frlg_test_input_tick(&state) == 1);
    assert(!frlg_test_input_accept(&state, p, sizeof(p)));
    assert(state.remaining == 1);
    assert(frlg_test_input_tick(&state) == 1);
    assert(frlg_test_input_tick(&state) == 0);
    p[12] = 2; p[16] = 16; p[18] = 120;
    assert(frlg_test_input_accept(&state, p, sizeof(p)));
    for (int i = 0; i < 120; i++) assert(frlg_test_input_tick(&state) == 16);
    assert(frlg_test_input_tick(&state) == 0);
    p[12] = 3; p[18] = 2;
    assert(frlg_test_input_accept(&state, p, sizeof(p)));
    p[12] = 4; p[16] = 0; p[18] = 0;
    assert(frlg_test_input_accept(&state, p, sizeof(p)));
    assert(frlg_test_input_tick(&state) == 0);
    p[12] = 5; p[16] = 1; p[18] = 2;
    const int offsets[] = {0,4,12,17,18,20};
    const int values[] = {'X',8,0,4,121,1};
    for (unsigned i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++) {
        uint8_t invalid[24]; memcpy(invalid,p,24); invalid[offsets[i]] = values[i];
        assert(!frlg_test_input_accept(&state,invalid,24));
        assert(state.sequence == 4 && state.remaining == 0);
    }
    assert(!frlg_test_input_accept(&state,p,23));
    assert(!frlg_test_input_accept(&state,p,25));
    assert(!frlg_test_input_accept(NULL,p,24));
    assert(!frlg_test_input_accept(&state,NULL,24));
    p[18] = 0;
    assert(!frlg_test_input_accept(&state,p,24));
    p[16] = 0; p[18] = 1;
    assert(!frlg_test_input_accept(&state,p,24));
    return 0;
}
