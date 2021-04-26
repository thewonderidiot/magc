//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "control.h"
#include "counter.h"
#include "scaler.h"

//---------------------------------------------------------------------------//
//                         Local Function Prototypes                         //
//---------------------------------------------------------------------------//
static void scaler_f06b(agc_state_t *state);
static void scaler_f07a(agc_state_t *state);
static void scaler_f07b(agc_state_t *state);
static void scaler_f09a(agc_state_t *state);
static void scaler_f09b(agc_state_t *state);
static void scaler_f10a(agc_state_t *state);
static void scaler_f10b(agc_state_t *state);
static void scaler_f12b(agc_state_t *state);
static void scaler_f14b(agc_state_t *state);
static void scaler_f16b(agc_state_t *state);
static void scaler_f17a(agc_state_t *state);
static void scaler_f17b(agc_state_t *state);

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
void scaler_advance(agc_state_t *state) {
    state->scaler_divider += SCALER_INCREMENT;

    if (state->scaler_divider < SCALER_OVERFLOW) {
        return;
    }

    state->scaler_divider -= SCALER_OVERFLOW;
    state->scaler++;

    uint16_t timing_bits = state->scaler & 0177777;
    uint8_t scaler_stage;
    if (timing_bits == 0) {
        scaler_stage = 19;
    } else {
        scaler_stage = __builtin_ctz(timing_bits) + 3;
    }

    // B pulses
    switch (scaler_stage) {
    case 6:
        scaler_f06b(state);
        break;
    case 7:
        scaler_f07b(state);
        break;
    case 9:
        scaler_f09b(state);
        break;
    case 10:
        scaler_f10b(state);
        break;
    case 12:
        scaler_f12b(state);
        break;
    case 14:
        scaler_f14b(state);
        break;
    case 16:
        scaler_f16b(state);
        break;
    case 17:
        scaler_f17b(state);
        break;
    }

    // A pulses
    switch (scaler_stage - 1) {
    default:
    case 17:
        scaler_f17a(state);
        // fallthrough
    case 16:
    case 15:
    case 14:
    case 13:
    case 12:
    case 11:
    case 10:
        scaler_f10a(state);
        // fallthrough
    case 9:
        scaler_f09a(state);
        // fallthrough
    case 8:
    case 7:
        scaler_f07a(state);
        // fallthrough
    case 6:
    case 5:
    case 4:
    case 3:
    case 2:
        break;
    }
}

//---------------------------------------------------------------------------//
//                        Local Function Definitions                         //
//---------------------------------------------------------------------------//
static void scaler_f06b(agc_state_t *state) {
    if (state->chan13 & 040000) {
        counter_request(state, COUNTER_TIME6, COUNT_DOWN);
    }
}

static void scaler_f07a(agc_state_t *state) {
    state->only_counts = state->inkl;
}

static void scaler_f07b(agc_state_t *state) {
    if (state->only_counts) {
        state->chan77 |= 0100;
    }
}

static void scaler_f09a(agc_state_t *state) {
    if (state->chan15 && !state->kyrpt1_set) {
        state->kyrpt1_pending = 1;
    }

    if ((state->chan16 & 037) && !state->kyrpt2_set) {
        state->kyrpt2_pending = 1;
    }

    if ((state->chan16 & 0140) && !state->mkrpt_set) {
        state->mkrpt_pending = 1;
    }
}

static void scaler_f09b(agc_state_t *state) {
    if (!(state->scaler & FS10)) {
        counter_request(state, COUNTER_TIME4, COUNT_UP);
    }

    if (state->kyrpt1_pending) {
        state->pending_rupts |= (1 << RUPT_KEYRUPT1);
        state->kyrpt1_set = 1;
        state->kyrpt1_pending = 0;
    }

    if (state->kyrpt2_pending) {
        state->pending_rupts |= (1 << RUPT_KEYRUPT2);
        state->kyrpt2_set = 1;
        state->kyrpt2_pending = 0;
    }

    if (state->mkrpt_pending) {
        state->pending_rupts |= (1 << RUPT_KEYRUPT2);
        state->mkrpt_set = 1;
        state->mkrpt_pending = 0;
    }
}

static void scaler_f10a(agc_state_t *state) {
    counter_request(state, COUNTER_TIME5, COUNT_UP);

    if (state->only_tc || state->no_tc) {
        control_gojam(state);
        state->chan77 |= 04;
    }
}

static void scaler_f10b(agc_state_t *state) {
    counter_request(state, COUNTER_TIME1, COUNT_UP);
    counter_request(state, COUNTER_TIME3, COUNT_UP);

    state->only_tc = 1;
    state->no_tc = 1;
}

static void scaler_f12b(agc_state_t *state) {
    if ((state->scaler & (FS14 | FS13)) == FS13) {
        if (state->only_rupt || state->no_rupt) {
            control_gojam(state);
            state->chan77 |= 010;
        }
    }
}

static void scaler_f14b(agc_state_t *state) {
    state->only_rupt = state->iip;
    state->no_rupt = !state->iip;
}

static void scaler_f16b(agc_state_t *state) {
    // Turn off flash (lights go on)
    state->flash = 0;
    state->dsky.vnflash = 0;
    if (state->chan11 & 0100) {
        state->dsky.oper_err = 1;
    }
    if (state->chan11 & 020) {
        state->dsky.key_rel = 1;
    }
}

static void scaler_f17a(agc_state_t *state) {
    // Turn on flash (lights go off)
    state->flash = 1;
    if (state->chan11 & 040) {
        state->dsky.vnflash = 1;
    }
    state->dsky.key_rel = 0;
    state->dsky.oper_err = 0;

    if (state->night_watchman) {
        control_gojam(state);
        state->chan77_watchman = 020;
    } else {
        state->chan77_watchman = 0;
    }

    if ((state->sbybut == SBYBUT_RELEASED) && !(state->chan32 & 020000)) {
        state->sbybut = SBYBUT_PRESSED;
    }
}

static void scaler_f17b(agc_state_t *state) {
    state->night_watchman = 1;

    if ((state->sbybut == SBYBUT_PRESSED) && (state->chan13 & 02000)) {
        state->sbybut = SBYBUT_TRIGGERED;
        // FIXME: HANDLE STANDBY
    }
}
