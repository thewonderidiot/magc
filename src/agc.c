//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include <stdio.h>
#include <errno.h>
#include "scaler.h"
#include "counter.h"
#include "subinst.h"
#include "control.h"
#include "utils.h"
#include "agc.h"

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
void agc_init(agc_state_t *state) {
    control_gojam(state);
    state->chan30 = 077777;
    state->chan31 = 037777;
    state->chan32 = 077777;
    state->chan33 = 077777;
}

void agc_service(agc_state_t *state) {
    scaler_advance(state);

    if (state->inkl) {
        counter_service(state);
    } else {
        state->only_counts = 0;
        // INKBT1
        if (state->st != 2) {
            state->nisql = 0;
            state->futext = 0;
        }
        state->gnhnc = 0;

        subinst_exec(state);
    }

    // T12
    if (state->pale) {
        control_gojam(state);
        state->chan77 |= 01;
        return;
    }
    state->st = state->st_pend;
    state->st_pend = 0;

    state->edit = 0;

    uint16_t a_sign = state->a & 0140000;
    uint8_t ovnhrp = (a_sign == 0100000) || (a_sign == 0040000);
    if (state->nisql) {
        if (!state->gnhnc && !state->pseudo && state->pending_counters) {
            state->inkl = 1;
        }

        if (state->pending_rupts && !state->inhint && !state->iip && !state->futext && !state->pseudo && !ovnhrp) {
            // RPTFRC
            state->sq = 00;
            state->qc = 03;
            state->sqr10 = 01;
            state->sqext = 1;
        } else {
            state->sq = ((state->b >> 12) & 03) | ((state->b >> 13) & 04);
            state->qc = (state->b >> 10) & 03;
            state->sqr10 = (state->b >> 9) & 01;
            state->sqext = state->futext;
        }
    }
}

int agc_load_rope(agc_state_t *state, char *rope_file) {
    FILE *fp = fopen(rope_file, "rb");
    if (fp == NULL) {
        return errno;
    }

    size_t num_words = fread(state->f, sizeof(state->f[0]), NUM_ELEMENTS(state->f), fp);

    fclose(fp);

    for (size_t i = 0; i < num_words; i++) {
        uint16_t word = state->f[i]; 
        word = (word >> 8) | ((word & 0xFF) << 8);
        state->tpgf[i] = check_parity(word);
        state->f[i] = (word & 0137777) | ((word >> 1) & 040000);
    }

    return 0;
}

void agc_set_chan15(agc_state_t *state, uint8_t keycode) {
    state->chan15 = keycode & 037;
    if (!state->chan15) {
        state->kyrpt1_pending = 0;
        state->kyrpt1_set = 0;
    }
}

void agc_set_chan16(agc_state_t *state, uint8_t keycode) {
    state->chan15 = keycode & 0177;
    if (!(keycode & 037)) {
        state->kyrpt2_pending = 0;
        state->kyrpt2_set = 0;
    }

    if (!(keycode & 0140)) {
        state->mkrpt_pending = 0;
        state->mkrpt_set = 0;
    }
}

void agc_set_chan32(agc_state_t *state, uint16_t value) {
    state->chan32 = value & 077777;
    if (value & 020000) {
        state->sbybut = SBYBUT_RELEASED;
    }
}
