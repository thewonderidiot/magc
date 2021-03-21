//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "dsky.h"

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
void dsky_set_chan15(agc_state_t *state, uint8_t keycode) {
    state->chan15 = keycode & 037;
    if (!keycode) {
        state->kyrpt1_pending = 0;
        state->kyrpt1_set = 0;
    }
}
