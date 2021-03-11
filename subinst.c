#include <stdio.h>
#include "control.h"
#include "mem.h"
#include "subinst.h"

#define X(_n,...) \
static void exec_##_n(agc_state_t *state);
SUBINSTRUCTIONS
#undef X

void subinst_exec(agc_state_t *state) {
    uint16_t op = (state->sqext << 3) | state->sq;
    uint16_t subinst_id = state->st;
    if (subinst_id != 2) {
        subinst_id |= (op << 6);
    
        switch (op) {
        case 001:
        case 011:
        case 016:
            subinst_id |= (state->qc > 0) << 4;
            break;

        case 010:
            subinst_id |= (state->sqr10) << 3;
            // fallthrough

        case 002:
        case 005:
        case 012:
            subinst_id |= (state->qc) << 4;
            break;
        }
    }

    switch (subinst_id) {
#define X(_n, ...) \
    case SUBINST_##_n: \
        exec_##_n(state); \
        printf("%s\n", #_n); \
        break;
    SUBINSTRUCTIONS
#undef X
    }
}

static void exec_TC0(agc_state_t *state) {
    // 1. RB WY12 CI
    uint16_t u = control_add(state->b & 07777, 1);
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // 3. RZ WQ
    state->q = state->z;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 6. RU WZ
    state->z = u;
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
}

static void exec_GOJ1(agc_state_t *state) {
    // 2. RSC WG (no effect)
    // 8. RSTRT WS WB
    state->s = 04000;
    state->b = 04000;
}

static void exec_TCSAJ3(agc_state_t *state) {
    // RSC WG (no effect)
    // WS WZ ST2
    state->s = state->mwl;
    state->z = state->mwl;
    state->st_pend |= 2;
}

static void exec_CCS0(agc_state_t *state) {
    // RL10BB WS
    state->s = state->b & 01777;
    // RSC WG
    state->g = control_rsc(state);
    // RG WB TSGN TMZ TPZG
    state->g |= state->mem;
    state->b = state->g;
    state->br = (state->g >> 15) & 01;
    if ((state->g == 0177777) || (state->g == 0)) {
        state->br |= 02;
    }

    // 00 RZ WY12
    // 01 RZ WY12 PONEX
    // 10 RZ WY12 PTWOX
    // 11 RZ WY12 PONEX PTWOX
    uint16_t u = state->z = control_add(state->z, state->br);
    // RU WZ WS
    state->z = u;
    state->s = u;

    switch (state->br) {
    case 0:
        // 00 RB WY MONEX CI ST2
        state->a = control_add(state->b, 0177776);
        break;
    case 1:
    case 3:
        // X1 WY ST2
        state->a = 0;
        break;
    case 2:
        // 10 RC WY MONEX CI ST2
        state->a = control_add(state->b ^ 0177777, 0177776);
        break;
    }

    state->st_pend |= 2;
}

static void exec_TCF0(agc_state_t *state) {
    // RB WY12 CI
    uint16_t u = control_add(state->b & 07777, 1);
    // RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // RU WZ
    state->z = u;
    // RAD W WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
}

static void exec_CA0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // Memory cycle completion
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. RB WA
    state->a = state->b;
}

static void exec_XCH0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB
    state->a = state->b;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WA
    state->a = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    state->g = state->b;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
}

static void exec_STD2(agc_state_t *state) {
    // 1. RZ WY12 CI
    uint16_t u = control_add(state->z & 07777, 1);
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 6. RU WZ
    state->z = u;
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
}
