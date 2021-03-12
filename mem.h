#ifndef _MEM_H_
#define _MEM_H_

#include "agc_state.h"

uint16_t mem_read(agc_state_t *state);
void mem_write(agc_state_t *state, uint16_t g);

#endif//_MEM_H_
