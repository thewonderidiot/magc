#ifndef _STATE_H_
#define _STATE_H_
//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include <stdint.h>
#include "hw_defs.h"

//---------------------------------------------------------------------------//
//                             Type Definitions                              //
//---------------------------------------------------------------------------//
typedef struct {
    uint16_t out0;
    uint8_t vnflash;
    uint8_t restart;
} dsky_t;

typedef struct {
    uint16_t a;
    uint16_t l;
    uint16_t q;
    uint16_t z;
    uint16_t feb;
    uint16_t fb;
    uint16_t eb;
    
    uint16_t g;
    uint16_t b;
    uint8_t futext;
    uint8_t sqext;
    uint8_t sq;
    uint8_t qc;
    uint8_t sqr10;
    uint16_t s;
    uint16_t u;

    uint8_t pseudo;
    uint8_t gnhnc;
    uint8_t iip;
    uint8_t inhint;
    uint8_t inkl;
    uint8_t st;
    uint8_t st_pend;
    uint8_t nisql;
    uint8_t edit;

    uint16_t e[ERASABLE_SIZE];
    uint16_t f[FIXED_SIZE];
    uint8_t tpgf[FIXED_SIZE];
    uint16_t writeback;

    uint8_t  scaler_divider;
    uint32_t scaler;

    uint8_t counters[NUM_COUNTERS];
    uint32_t pending_counters;
    uint16_t pending_rupts;

    uint16_t mwl;

    uint8_t chan5;
    uint8_t chan6;
    uint16_t chan11;
    uint16_t chan12;
    uint16_t chan13;
    uint16_t chan14;
    uint8_t chan15;
    uint8_t chan16;
    uint16_t chan30;
    uint16_t chan31;
    uint16_t chan32;
    uint16_t chan33;

    uint8_t kyrpt1_pending;
    uint8_t kyrpt1_set;

    dsky_t dsky;
} agc_state_t;

#endif//_STATE_H_
