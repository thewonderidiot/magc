//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "counter.h"
#include "scaler.h"

//---------------------------------------------------------------------------//
//                         Local Function Prototypes                         //
//---------------------------------------------------------------------------//
static void scaler_f09a(agc_state_t *state);
static void scaler_f09b(agc_state_t *state);
static void scaler_f10a(agc_state_t *state);
static void scaler_f10b(agc_state_t *state);

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
    case 9:
        scaler_f09b(state);
        break;
    case 10:
        scaler_f10b(state);
        break;
    }

    // A pulses
    switch (scaler_stage - 1) {
    default:
    case 10:
        scaler_f10a(state);
        // fallthrough
    case 9:
        scaler_f09a(state);
        // fallthrough
    case 8:
    case 7:
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
static void scaler_f09a(agc_state_t *state) {
    if (state->chan15 && !state->kyrpt1_set) {
        state->kyrpt1_pending = 1;
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
}

static void scaler_f10a(agc_state_t *state) {
    counter_request(state, COUNTER_TIME5, COUNT_UP);
}

static void scaler_f10b(agc_state_t *state) {
    counter_request(state, COUNTER_TIME1, COUNT_UP);
    counter_request(state, COUNTER_TIME3, COUNT_UP);
}
