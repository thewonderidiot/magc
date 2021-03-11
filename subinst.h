#ifndef _SUBINST_H_
#define _SUBINST_H_

#include "agc_state.h"

#define SUBINSTRUCTIONS \
    /* Name    OP,  QC, IO, ST */ \
    X( TC0,    000,  0,  0,  0 ) \
    X( GOJ1,   000,  0,  0,  1 ) \
    X( TCSAJ3, 000,  0,  0,  3 ) \
    X( CCS0,   001,  0,  0,  0 ) \
    X( TCF0,   001,  1,  0,  0 ) \
    X( CA0,    003,  0,  0,  0 ) \
    X( XCH0,   005,  3,  0,  0 ) \
    X( STD2,   000,  0,  0,  2 ) \

typedef enum {
#define X(_n, _o, _q, _i, _s, ...) \
    SUBINST_##_n = (_o << 6) | (_q << 4) | (_i << 3) | _s,
    SUBINSTRUCTIONS
#undef X
} subinst_id_t;

void subinst_exec(agc_state_t *state);

#endif//_SUBINST_H_
