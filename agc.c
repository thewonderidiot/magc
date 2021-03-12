#include <stdio.h>
#include "subinst.h"
#include "control.h"
#include "utils.h"
#include "agc.h"

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
        state->sq = ((state->b >> 12) & 03) | ((state->b >> 13) & 04);
        state->qc = (state->b >> 10) & 03;
        state->sqr10 = (state->b >> 9) & 01;
        state->sqext = state->futext;
    }

    state->st = state->st_pend;
    state->st_pend = 0;
}

void agc_load_rope(agc_state_t *state, char *rope_file) {
    FILE *fp = fopen(rope_file, "rb");
    size_t num_words = fread(state->f, sizeof(state->f[0]), NUM_ELEMENTS(state->f), fp);
    fclose(fp);

    for (size_t i = 0; i < num_words; i++) {
        uint16_t word = state->f[i]; 
        word = (word >> 8) | ((word & 0xFF) << 8);
        state->tpgf[i] = check_parity(word);
        state->f[i] = (word & 0137777) | ((word >> 1) & 040000);
    }
}
