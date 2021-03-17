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
    X( DAS0,   002,  0,  0,  0 ) \
    X( DAS1,   002,  0,  0,  1 ) \
    X( LXCH0,  002,  1,  0,  0 ) \
    X( INCR0,  002,  2,  0,  0 ) \
    X( ADS0,   002,  3,  0,  0 ) \
    X( CA0,    003,  0,  0,  0 ) \
    X( CS0,    004,  0,  0,  0 ) \
    X( NDX0,   005,  0,  0,  0 ) \
    X( NDX1,   005,  0,  0,  1 ) \
    X( RSM3,   005,  0,  0,  3 ) \
    X( DXCH0,  005,  1,  0,  0 ) \
    X( DXCH1,  005,  1,  0,  1 ) \
    X( TS0,    005,  2,  0,  0 ) \
    X( XCH0,   005,  3,  0,  0 ) \
    X( AD0,    006,  0,  0,  0 ) \
    X( MSK0,   007,  0,  0,  0 ) \
    X( WAND0,  010,  1,  1,  0 ) \
    X( DV0,    011,  0,  0,  0 ) \
    X( DV1,    011,  0,  0,  1 ) \
    X( DV3,    011,  0,  0,  3 ) \
    X( DV7,    011,  0,  0,  7 ) \
    X( DV6,    011,  0,  0,  6 ) \
    X( BZF0,   011,  1,  0,  0 ) \
    X( MSU0,   012,  0,  0,  0 ) \
    X( QXCH0,  012,  1,  0,  0 ) \
    X( AUG0,   012,  2,  0,  0 ) \
    X( DIM0,   012,  3,  0,  0 ) \
    X( DCA0,   013,  0,  0,  0 ) \
    X( DCA1,   013,  0,  0,  1 ) \
    X( DCS0,   014,  0,  0,  0 ) \
    X( DCS1,   014,  0,  0,  1 ) \
    X( SU0,    016,  0,  0,  0 ) \
    X( BZMF0,  016,  1,  0,  0 ) \
    X( MP0,    017,  0,  0,  0 ) \
    X( MP1,    017,  0,  0,  1 ) \
    X( MP3,    017,  0,  0,  3 ) \
    X( STD2,   000,  0,  0,  2 ) \

typedef enum {
#define X(_n, _o, _q, _i, _s, ...) \
    SUBINST_##_n = (_o << 6) | (_q << 4) | (_i << 3) | _s,
    SUBINSTRUCTIONS
#undef X
} subinst_id_t;

void subinst_exec(agc_state_t *state);

#endif//_SUBINST_H_
