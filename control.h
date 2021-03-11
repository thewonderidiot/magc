#ifndef _CONTROL_H_
#define _CONTROL_H_

#include "agc_state.h"

void control_gojam(agc_state_t *state);
uint16_t control_add(uint16_t x, uint16_t y);
uint16_t control_rad(agc_state_t *state);
uint16_t control_rsc(agc_state_t *state);
void control_wg(agc_state_t *state, uint16_t wl);
void control_wsc(agc_state_t *state, uint16_t wl);

#endif//_CONTROL_H_
