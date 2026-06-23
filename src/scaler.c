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
    // The scaler is a 33-bit counter that increments once every 9.765625us.
    // For emulation, we only need to care about bit 3 and higher, giving
    // us a counter period of 9.765625us * 2^2 = 39.0625us. The basic time
    // unit of the emulator is the MCT (11.71875us), which works out to be
    // exactly 3/10th of our desired scaler period. So, each MCT we add 3
    // to an internal counter, and increment the scaler and subtract 10
    // once 10 is reached or surpassed.
    state->scaler_divider += SCALER_INCREMENT;
    if (state->scaler_divider < SCALER_OVERFLOW) {
        return;
    }

    state->scaler_divider -= SCALER_OVERFLOW;
    state->scaler++;

    // Most of the timing in the AGC is derived from fixed-width ~10us "A" and
    // "B" frequency, occur on falling and rising edges of their corresponding
    // scaler bits, respectively. So for example, F03A occurs when bit 3 of
    // the scaler changes to 0, and F03B occurs when it changes to 1.
    // F03B is the fastest pulse we need to worry about, while F18A/F18B are
    // the slowest. Since FS03 is our emulated scaler's LSB, this means we
    // need to examine the lower 16 bits.
    uint16_t timing_bits = state->scaler & 0177777;

    // We can quickly determine which timing pulses need to fire by examining
    // the number of trailing zeros in our scaler. Consider the increment:
    //    001111 -> 010000
    // Here, the new scaler value has 4 trailing zeros. Based on the trailing
    // zero count being 4, we can say:
    //   * Bit 5 must have just changed from 0 to 1 (B pulse).
    //   * Bits 1-4 must have all just changed from 1 to 0 (A pulses).
    // Since our LSB is stage 3, we can add 3 to the trailing zero count
    // to identify the stage receiving a B pulse this cycle. Likewise,
    // we can subtract 1 from that to identify all stages requiring
    // A pulses.
    uint8_t scaler_stage;
    if (timing_bits == 0) {
        // If all of our timing bits are zero, FS19 must have just gone high.
        scaler_stage = 19;
    } else {
        // Count trailing zeros and add 3 to identify the needed B pulse.
        scaler_stage = __builtin_ctz(timing_bits) + 3;
    }

    // Execute the appropriate B pulse for this scaler count.
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

    // Execute all A pulses for this scaler count (starting one frequency
    // faster than whatever B pulse we may have just done).
    switch (scaler_stage - 1) {
    default:
    case 19:
    case 18:
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
    // Generate a DINC request for TIME6 if bit 15 of channel 13 is set.
    if (state->chan13 & 040000) {
        counter_request(state, COUNTER_TIME6, COUNT_DOWN);
    }
}

static void scaler_f07a(agc_state_t *state) {
    // Start off the counter alarm cycle by latching the state of INKL
    // (which indicates counter cycles are in progress).
    state->only_counts = state->inkl;
}

static void scaler_f07b(agc_state_t *state) {
    // Finish the counter alarm cycle. If we have only executed counts
    // since F07A, set bit 7 of channel 77 and geneate an input to
    // the warning filter.
    if (state->only_counts) {
        state->chan77 |= 0100;
        // FIXME: Generate a warning filter input
    }
}

static void scaler_f09a(agc_state_t *state) {
    // Check to see if there is a main DSKY keypress input that has
    // not yet been processed.
    if (state->chan15 && !state->kyrpt1_set) {
        state->kyrpt1_pending = 1;
    }

    // Check to see if there is a nav DSKY keypress input that has
    // not yet been processed.
    if ((state->chan16 & 037) && !state->kyrpt2_set) {
        state->kyrpt2_pending = 1;
    }

    // Check to see if there is a mark button keypress input that has
    // not yet been processed.
    if ((state->chan16 & 0140) && !state->mkrpt_set) {
        state->mkrpt_pending = 1;
    }
}

static void scaler_f09b(agc_state_t *state) {
    // Generate a PINC for TIME4 if FS10 is not high.
    if (!(state->scaler & FS10)) {
        counter_request(state, COUNTER_TIME4, COUNT_UP);
    }

    // If we have a pending main DSKY keypress, trigger a KEYRUPT1
    // interrupt and set a flip-flop preventing further interrupts
    // until the key is released.
    if (state->kyrpt1_pending) {
        state->pending_rupts |= (1 << RUPT_KEYRUPT1);
        state->kyrpt1_set = 1;
        state->kyrpt1_pending = 0;
    }

    // If we have a pending nav DSKY keypress, trigger a KEYRUPT2
    // interrupt and set a flip-flop preventing further interrupts
    // until the key is released.
    if (state->kyrpt2_pending) {
        state->pending_rupts |= (1 << RUPT_KEYRUPT2);
        state->kyrpt2_set = 1;
        state->kyrpt2_pending = 0;
    }

    // If we have a pending mark button press, trigger a KEYRUPT2
    // interrupt and set a flip-flop preventing further interrupts
    // until the key is released.
    if (state->mkrpt_pending) {
        state->pending_rupts |= (1 << RUPT_KEYRUPT2);
        state->mkrpt_set = 1;
        state->mkrpt_pending = 0;
    }
}

static void scaler_f10a(agc_state_t *state) {
    // Generate a PINC count for TIME5
    counter_request(state, COUNTER_TIME5, COUNT_UP);

    // If either TC trap flip-flop is still set, trigger a GOJAM and
    // set bit 3 in channel 77.
    if (state->only_tc || state->no_tc) {
        control_gojam(state);
        state->chan77 |= 04;
    }
}

static void scaler_f10b(agc_state_t *state) {
    // Generate PINC requests for TIME1 and TIME3
    counter_request(state, COUNTER_TIME1, COUNT_UP);
    counter_request(state, COUNTER_TIME3, COUNT_UP);

    // Set flip-flops for both TC trap cases, which will be reset
    // (or not) depending on which instructions get executed.
    state->only_tc = 1;
    state->no_tc = 1;
}

static void scaler_f12b(agc_state_t *state) {
    // If FS13 is high but FS14 is not, finish the RUPT lock alarm cycle.
    // If either flip-flop is still set, generate a GOJAM and set bit 4
    // in channel 77.
    if ((state->scaler & (FS14 | FS13)) == FS13) {
        if (state->only_rupt || state->no_rupt) {
            control_gojam(state);
            state->chan77 |= 010;
        }
    }
}

static void scaler_f14b(agc_state_t *state) {
    // Start the RUPT lock alarm cycle by setting two flip-flops according
    // to whether or not we are currently in an interrupt.
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

    // If the night watchman flip-flop is still set, trigger a GOJAM and
    // set bit 5 of channel 77. This channel 77 bit is unique in that it
    // will continue to be asserted until the next F17A, even if the channel
    // is cleared.
    if (state->night_watchman) {
        control_gojam(state);
        state->chan77_watchman = 020;
    } else {
        state->chan77_watchman = 0;
    }

    // Check to see if the SBY (PRO) button has been pressed. If so, it must
    // be held until F17B for the computer to enter standby mode.
    if ((state->sbybut == SBYBUT_RELEASED) && !(state->chan32 & 020000)) {
        state->sbybut = SBYBUT_PRESSED;
    }
}

static void scaler_f17b(agc_state_t *state) {
    // Start the night watchman cycle.
    state->night_watchman = 1;

    // If the standby button has been pressed since F17A and bit 11 of channel
    // 13 is set, enter standby mode.
    if ((state->sbybut == SBYBUT_PRESSED) && (state->chan13 & 02000)) {
        state->sbybut = SBYBUT_TRIGGERED;
        // FIXME: HANDLE STANDBY
    }
}
