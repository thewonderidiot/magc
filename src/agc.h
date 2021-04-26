#ifndef _AGC_H_
#define _AGC_H_
//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "agc_state.h"

//---------------------------------------------------------------------------//
//                        Global Function Prototypes                         //
//---------------------------------------------------------------------------//
void agc_init(agc_state_t *state);
void agc_service(agc_state_t *state);
int agc_load_rope(agc_state_t *state, char *rope_file);
void agc_set_chan15(agc_state_t *state, uint8_t keycode);
void agc_set_chan16(agc_state_t *state, uint8_t keycode);
void agc_set_chan32(agc_state_t *state, uint16_t value);

#endif//_AGC_H_
