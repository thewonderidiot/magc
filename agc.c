//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include <stdio.h>
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
}

void agc_service(agc_state_t *state) {
    scaler_advance(state);

    if (state->inkl) {
        counter_service(state);
    } else {
        // INKBT1
        if (state->st != 2) {
            state->nisql = 0;
            state->futext = 0;
        }
        state->gnhnc = 0;

        subinst_exec(state);
    }

    // T12
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
