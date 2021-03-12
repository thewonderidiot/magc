#include "mem.h"

uint16_t mem_read(agc_state_t *state) {
    uint16_t addr;
    uint16_t val;

    if (state->s < 010) {
        return 0;
    }

    if (state->s < 02000) {
        if (state->s < 01400) {
            addr = state->s;
        } else {
            addr = state->eb | (state->s & ~01400);
        }
        val = state->e[addr];
        state->e[addr] = 0;
        state->writeback = addr;
    } else {
        // FIXME: check parity
        if (state->s < 04000) {
            addr = state->fb | (state->s & ~02000);
        } else {
            addr = state->s;
        }
        val = state->f[addr];
    }

    return val;
}

void mem_write(agc_state_t *state, uint16_t g) {
    if (state->writeback) {
        state->e[state->writeback] = (g & 0137777) | ((g >> 1) & 040000);
        state->writeback = 0;
    }
}
