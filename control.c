#include "control.h"

void control_gojam(agc_state_t *state) {
    state->nisql = 0;
    state->sqext = 0;
    state->sq = 0;
    state->qc = 0;
    state->sqr10 = 0;
    state->st = 1;
    state->st_pend = 0;
    state->iip = 0;
    state->inhint = 0;
}

uint16_t control_add(uint16_t x, uint16_t y) {
    uint32_t u = (uint32_t)x + (uint32_t)y;
    u += (u >> 16) & 1;
    return (u & 0177777);
}

uint16_t control_rad(agc_state_t *state) {
    switch (state->g) {
    case 000003:
        state->inhint = 0;
        state->st_pend = 2;
        return state->z;
    case 000004:
        state->inhint = 1;
        state->st_pend = 2;
        return state->z;
    case 000006:
        state->sqext = 1;
        state->st_pend = 2;
        return state->z;
    default:
        return state->g;
    }
}

uint16_t control_rsc(agc_state_t *state) {
    switch (state->s) {
    case 0: 
        return state->a;
    case 1:
        return state->l & 077777;
    case 2:
        return state->q;
    case 3:
        return state->eb;
    case 4:
        return state->fb;
    case 5:
        return state->z;
    case 6:
        return (state->eb >> 8) | state->fb;
    case 7:
        return 0;
    default:
        return 0;
    }
}

void control_wsc(agc_state_t *state, uint16_t wl) {
    // FIXME
}

void control_wg(agc_state_t *state, uint16_t wl) {
    switch (state->s) {
    default:
        state->g = wl;
        break;
    }
}
