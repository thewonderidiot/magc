#include "mem.h"

uint16_t mem_read(agc_state_t *state) {
    if (state->s < 010) {
        return 0;
    }

    if (state->s < 01400) {
        return state->e[state->s];
    } else if (state->s < 02000) {
        uint16_t eaddr = state->eb | (state->s & ~01400);
        return state->e[eaddr];
    } else if (state->s < 04000) {
        uint16_t faddr = state->fb | (state->s & ~02000);
        return state->f[faddr];
    } else {
        return state->f[state->s];
    }
}
