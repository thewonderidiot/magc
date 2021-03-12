#ifndef _AGC_H_
#define _AGC_H_

#include "agc_state.h"

void agc_init(agc_state_t *state);
void agc_service(agc_state_t *state);
void agc_load_rope(agc_state_t *state, char *rope_file);

#endif//_AGC_H_
