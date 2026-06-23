//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "subinst.h"
#include "counter.h"

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
void counter_request(agc_state_t *state, counter_t counter, count_type_t type) {
    // Set the flip-flop corresponding to the direction of the cound, and
    // mark this counter as pending on the priority chain.
    state->counters[counter] |= type;
    state->pending_counters |= (1 << counter);
}

void counter_service(agc_state_t *state) {
    // Inspect the priority chain to determine the highest priority pending
    // counter, and determine its address.
    uint8_t counter = __builtin_ffs(state->pending_counters) - 1;
    uint16_t addr = COUNTER_BASE_ADDR + counter;

    // Exceute the correct type of unprogrammed count instruction, depending
    // upon the specific counter and the direction flip-flops.
    switch (counter) {
    case COUNTER_TIME2:
    case COUNTER_TIME1:
    case COUNTER_TIME3:
    case COUNTER_TIME4:
    case COUNTER_TIME5:
        exec_PINC(state, addr);
        break;

    case COUNTER_TIME6:
        exec_DINC(state, addr);
        break;
    }

    // Clear the direction flip-flops and remove this counter from the priority
    // chain.
    state->counters[counter] = 0;
    state->pending_counters &= ~(1 << counter);

    // If this was the last pending count, unset INKL so programmed instrucitons
    // can continue.
    if (!state->pending_counters) {
        state->inkl = 0;
    }
}
