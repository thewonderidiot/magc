#include "control.h"

void control_gojam(agc_state_t *state) {
    state->nisql = 0;
    state->futext = 0;
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
        state->futext = 1;
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
        return (state->l & 0137777) | ((state->l >> 1) & 040000);
    case 2:
        return state->q;
    case 3:
        return state->eb;
    case 4:
        return state->fb | ((state->fb << 1) & 0100000);
    case 5:
        return state->z;
    case 6:
        return (state->eb >> 8) | state->fb | ((state->fb << 1) & 0100000);
    case 7:
        return 0;
    default:
        return 0;
    }
}

void control_wsc(agc_state_t *state, uint16_t wl) {
    switch (state->s) {
    case 0: 
        state->a = wl;
        break;
    case 1:
        state->l = wl;
        break;
    case 2:
        state->q = wl;
        break;
    case 3:
        state->eb = wl & 03400;
        break;
    case 4:
        state->fb = ((wl >> 1) & 040000) | (wl & 036000);
        break;
    case 5:
        state->z = wl;
        break;
    case 6:
        state->eb = (wl << 8) & 03400;
        state->fb = ((wl >> 1) & 040000) | (wl & 036000);
        break;
    default:
        break;
    }
}

void control_wg(agc_state_t *state, uint16_t wl) {
    switch (state->s) {
    // FIXME: EDIT
    default:
        state->g = wl;
        break;
    }
}
