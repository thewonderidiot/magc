#include "subinst.h"
#include "control.h"

void agc_init(agc_state_t *state) {
    control_gojam(state);

    for (uint16_t i = 0; i < 2048; i++) {
        state->e[i] = 0;
    }
}

void agc_service(agc_state_t *state) {
    // Check for counter pulses

    // INKBT1
    subinst_exec(state);

    if (state->nisql) {
        state->nisql = 0;
        state->sq = state->b >> 12;
        state->qc = (state->b >> 10) & 03;
        state->sqr10 = (state->b >> 9) & 01;
        state->sqext = 0;
    }

    state->st = state->st_pend;
    state->st_pend = 0;
}
