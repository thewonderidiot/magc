#ifndef _CONTROL_H_
#define _CONTROL_H_
//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "agc_state.h"

//---------------------------------------------------------------------------//
//                        Global Function Prototypes                         //
//---------------------------------------------------------------------------//
void control_gojam(agc_state_t *state);
uint16_t control_add(uint16_t x, uint16_t y, uint16_t ci);
uint16_t control_rad(agc_state_t *state);
uint16_t control_rsc(agc_state_t *state);
void control_wg(agc_state_t *state, uint16_t wl);
void control_wsc(agc_state_t *state, uint16_t wl);
uint16_t control_rch(agc_state_t *state);
void control_wch(agc_state_t *state, uint16_t wl);
void control_zip(agc_state_t *state);
void control_zap(agc_state_t *state);
void control_pout(agc_state_t *state);
void control_mout(agc_state_t *state);
void control_zout(agc_state_t *state);
void control_wovr(agc_state_t *state, uint16_t wl);

#endif//_CONTROL_H_
