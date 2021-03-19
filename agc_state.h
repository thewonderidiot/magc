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
    uint16_t a;
    uint16_t l;
    uint16_t q;
    uint16_t z;
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
} agc_state_t;

#endif//_STATE_H_
