//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "subinst.h"
#include "counter.h"

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
void counter_request(agc_state_t *state, counter_t counter, count_type_t type) {
    state->counters[counter] |= type;
    state->pending_counters |= (1 << counter);
}

void counter_service(agc_state_t *state) {
    uint8_t counter = __builtin_ffs(state->pending_counters) - 1;
    uint16_t addr = COUNTER_BASE_ADDR + counter;
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

    state->counters[counter] = 0;
    state->pending_counters &= ~(1 << counter);
    if (!state->pending_counters) {
        state->inkl = 0;
    }
}
