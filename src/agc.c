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
    state->chan30 = 037777;
    state->chan31 = 077777;
    state->chan32 = 077777;
    state->chan33 = 077777;
}

void agc_service(agc_state_t *state) {
    // Begin by advancing the scaler. This may or may not trigger a GOJAM.
    scaler_advance(state);

    // Check to see if a count cycle has been requested.
    if (state->inkl) {
        // There's a pending count. Service that and bypass most processing this MCT.
        counter_service(state);
    } else {
        // No counts this MCT. Reset the the counter alarm.
        state->only_counts = 0;

        // Generate INKBT1 -- if we're not in stage 2, reset NISQL and FUTEXT.
        if (state->st != 2) {
            state->nisql = 0;
            state->futext = 0;
        }

        // We've made it far enough into the MCT to stop GNHNC from inhibiting counts.
        state->gnhnc = 0;

        // All is good to go -- execute this MCT's subinstruction.
        subinst_exec(state);
    }

    // Perform T12 operations to round out the MCT.
    // Check for a parity alarm on the last access, and generate a GOJAM if one occurred.
    if (state->pale) {
        control_gojam(state);
        state->chan77 |= 01;
        return;
    }

    // Advance to the next selected stage
    state->st = state->st_pend;
    state->st_pend = 0;

    // Reset the editing flip-flops
    state->edit = 0;

    // Set OVNHRP if overflow or underflow exists in A
    uint16_t a_sign = state->a & 0140000;
    uint8_t ovnhrp = (a_sign == 0100000) || (a_sign == 0040000);

    // If NISQL is set, perform instruction changeover logic.
    if (state->nisql) {
        // Set INKL if:
        // * GNHNC is not inhibiting counts from the last GOJAM
        // * The next instruction is not a pseudoinstruction
        // * There are pending counts
        if (!state->gnhnc && !state->pseudo && state->pending_counters) {
            state->inkl = 1;
        }

        // Generate RPTFRC if:
        // * There are pending interrupts
        // * INHINT is not active
        // * IIP is not set (i.e. there's not already an active interrupt)
        // * FUTEXT is not set
        // * The next instruction is not a pseudoinstruction
        // * OVNHRP wasn't set above
        if (state->pending_rupts && !state->inhint && !state->iip && !state->futext && !state->pseudo && !ovnhrp) {
            // RPTFRC -- set all of the instruction selection registers for RUPT0
            state->sq = 00;
            state->qc = 03;
            state->sqr10 = 01;
            state->sqext = 1;
        } else {
            // No interrupt. Grab SQ from B, and SQEXT from FUTEXT.
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

    // Read the rope file into fixed memory
    size_t num_words = fread(state->f, sizeof(state->f[0]), NUM_ELEMENTS(state->f), fp);

    fclose(fp);

    for (size_t i = 0; i < num_words; i++) {
        // Swap endianness on each word
        uint16_t word = state->f[i]; 
        word = (word >> 8) | ((word & 0xFF) << 8);
        // Pre-calculate whether each word will generate a parity alarm when accessed
        state->tpgf[i] = check_parity(word);
        state->f[i] = (word & 0137777) | ((word >> 1) & 040000);
    }

    return 0;
}

void agc_set_chan15(agc_state_t *state, uint8_t keycode) {
    // Update channel 15 for software
    state->chan15 = keycode & 037;

    // If it's been set to 0, reset the KYRPT1 flip-flops
    if (!state->chan15) {
        state->kyrpt1_pending = 0;
        state->kyrpt1_set = 0;
    }
}

void agc_set_chan16(agc_state_t *state, uint8_t keycode) {
    // Update channel 16 for software
    state->chan16 = keycode & 0177;

    // If bits 1-5 have been set to 0, reset the KYRPT2 flip-flops
    if (!(keycode & 037)) {
        state->kyrpt2_pending = 0;
        state->kyrpt2_set = 0;
    }

    // If bits 6-7 have been set to 0, rset the MKRPT flip-flops
    if (!(keycode & 0140)) {
        state->mkrpt_pending = 0;
        state->mkrpt_set = 0;
    }
}

void agc_set_chan32(agc_state_t *state, uint16_t value) {
    // Update channel 32 for software
    state->chan32 = value & 077777;

    // If bit 14 is high, reset the standby button state machine
    if (value & 020000) {
        state->sbybut = SBYBUT_RELEASED;
    }
}
