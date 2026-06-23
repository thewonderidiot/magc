//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "mem.h"

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
uint16_t mem_read(agc_state_t *state) {
    uint16_t addr;
    uint16_t val;

    // If S is 00-07, then SCAD is set for Special/Central Addresses. Memory
    // cycles are inhibited; instead RSC will be used to read a register.
    if (state->s < 010) {
        return 0;
    }

    // Broadly speaking, erasable memory occupies the S address space 0010-1777,
    // while fixed memory occupies 2000-7777.
    if (state->s < 02000) {
        if (state->s < 01400) {
            // Addresses 0010-1400 are fixed-erasable, in which case we
            // can just use S as the overall erasable address.
            addr = state->s;
        } else {
            // Addresses 1400-1777 are switched-erasable. For these, bits 9-11
            // are replaced with the EB register to determine the full address.
            addr = state->eb | (state->s & ~01400);
        }

        // Perform the read on the fully resolved address, zero it out (since
        // reads are destructive), and save the address for writeback later
        // in the cycle.
        val = state->e[addr];
        state->e[addr] = 0;
        state->writeback = addr;

        // FIXME: is this correct enough?
        if (addr == 067) {
            state->night_watchman = 0;
        }
    } else {
        if (state->s < 04000) {
            // Addresses 2000-3777 correspond to switched-fixed memory.
            if ((state->fb >= 060000) && (state->feb & 0100000)) {
                // FB is >= 30 and the top FEB is set, so we're accessing a superbank.
                // The three FEB bits, followed by the lower three FB bits, make up
                // the top of a 16-bit address, allowing access to banks 40-77.
                addr = state->feb | (state->fb & 016000);
            } else {
                // We're not addressing superbanks, so we can use FB directly for the
                // upper address bits.
                addr = state->fb;
            }
            addr |= state->s & ~02000;
        } else {
            // Fixed-fixed memory spans 4000-7777. We can use S directly for this.
            addr = state->s;
        }

        // Read in the word at this location and check the pre-computed parity. If it
        // is not odd, set PALE to trigger a restart later in the cycle.
        val = state->f[addr];
        if (!state->tpgf[addr]) {
            state->pale = 1;
        }
    }

    return val;
}

void mem_write(agc_state_t *state, uint16_t g) {
    // If this was an erasable cycle and our current switch cores have saved an
    // address, write the current value of G back out to that location.
    if (state->writeback) {
        state->e[state->writeback] = (g & 0137777) | ((g >> 1) & 040000);
        state->writeback = 0;
    }
}
