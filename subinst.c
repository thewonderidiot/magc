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
        printf("%s\n", #_n); \
        exec_##_n(state); \
        break;
    SUBINSTRUCTIONS
#undef X
    default:
        printf("UNKNOWN INSTRUCTION\n");
    }
}

static void exec_TC0(agc_state_t *state) {
    // 1. RB WY12 CI
    uint16_t u = control_add(state->b & 07777, 0, 1);
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
    // Memory cycle writeback
    mem_write(state, state->g);
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
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WB TSGN TMZ TPZG
    state->b = state->g;
    uint16_t br = (state->g >> 14) & 02;
    if ((state->g == 0177777) || (state->g == 0)) {
        br |= 01;
    }
    // 7. 00 RZ WY12
    // 7. 01 RZ WY12 PONEX
    // 7. 10 RZ WY12 PTWOX
    // 7. 11 RZ WY12 PONEX PTWOX
    uint16_t u = state->z = state->z + br;
    // 8. RU WZ WS
    state->z = u;
    state->s = u;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. 00 RB WY MONEX CI ST2
    // 10. X1 WY ST2
    // 10. 10 RC WY MONEX CI ST2
    switch (br) {
    case 0:
        u = control_add(state->b, 0177776, 1);
        break;
    case 1:
    case 3:
        u = 0;
        break;
    case 2:
        u = control_add(state->b ^ 0177777, 0177776, 1);
        break;
    }
    state->st_pend |= 2;
    // Memory cycle writeback
    mem_write(state, state->g);
    // 11. RU WA
    state->a = u;
}

static void exec_TCF0(agc_state_t *state) {
    // 1. RB WY12 CI
    uint16_t u = control_add(state->b & 07777, 0, 1);
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
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_LXCH0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RL WB
    state->b = (state->l & 0137777) | ((state->l >> 1) & 040000);
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WL
    state->l = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_INCR0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WY TSGN TMZ TPZG
    // 6. PONEX
    uint16_t u = control_add(state->g, 1, 0);
    // 7. RU WSC WG WOVR
    control_wsc(state, u);
    control_wg(state, u);
    // FIXME: WOVR
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_ADS0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WY A2X
    uint16_t u = control_add(state->g, state->a, 0);
    // 6. RU WSC WG TOV
    control_wsc(state, u);
    control_wg(state, u);
    // 7. 00 WA (no effect)
    // 7. 01 WA RB1 (no effect)
    // 7. 10 WA R1C (no effect)
    // 7. 11 WA (no effect)
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RC TMZ (no effect)
    // 10. RU WA
    state->a = u;
    // Memory cycle writeback
    mem_write(state, state->g);
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
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_CS0(agc_state_t *state) {
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
    // 10. RC WA
    state->a = state->b ^ 0177777;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_NDX0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. TRSM
    if (state->s == 00017) {
        state->st_pend = 2;
    }
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS
    state->s = state->z & 07777;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. ST1
    state->st_pend |= 1;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_NDX1(agc_state_t *state) {
    // 1. RZ WY12 CI
    uint16_t u = (state->z & 07777) + 1;
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // 3. RB WZ
    state->z = state->b;
    // 4. RA WB
    state->b = state->a;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RZ WA
    state->a = state->z;
    // 6. RU WZ
    state->z = u;
    // 7. RG WY A2X
    u = control_add(state->g, state->a, 0);
    // 8. RU WS
    state->s = u & 07777;
    // 9. RB WA
    state->a = state->b;
    // 10. RU WB
    state->b = u;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_RSM3(agc_state_t *state) {
    // 1. R15 WS
    state->s = 015;
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WZ
    state->z = state->g;
    // 6. RB WG
    state->g = state->b;
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_DXCH0(agc_state_t *state) {
    // 1. RL10BB WS WY12 MONEX CI
    state->s = state->b & 01777;
    uint16_t u = control_add(state->s, 0177776, 1);
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RL WB
    state->b = (state->l & 0137777) | ((state->l >> 1) & 040000);
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WL
    state->l = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RU WS WB
    state->s = u;
    state->b = u;
    // Memory cycle writeback
    mem_write(state, state->g);
    // 10. ST1
    state->st_pend = 1;
}

static void exec_DXCH1(agc_state_t *state) {
    // 1. RL10BB WS (no effect)
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB
    state->b = state->a;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WA
    state->a = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_TS0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB TOV
    state->b = state->a;
    uint16_t sign = state->b & 0140000;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 4. 00 RZ WY12
    // 4. 01 RZ WY12 CI
    // 4. 10 RZ WY12 CI
    // 4. 11 RZ WY12
    // 5. 01 RB1 WA
    // 5. 10 R1C WA
    uint16_t u = state->z & 07777;
    if (sign == 0040000) {
        u++;
        state->a = 01;
    } else if (sign == 0100000) {
        u++;
        state->a = 0177776;
    }
    // 6. RU WZ
    state->z = u;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_XCH0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB
    state->b = state->a;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WA
    state->a = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_AD0(agc_state_t *state) {
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
    // Memory cycle writeback
    mem_write(state, state->g);
    // 10. RB WY A2X
    uint16_t u = control_add(state->b, state->a, 0);
    // 11. RU WA
    state->a = u;
}

static void exec_MSK0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB
    state->b = state->a;
    // 4. RC WA
    state->a = state->b ^ 0177777;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RC RA WY
    uint16_t u = (state->b ^ 0177777) | state->a;
    // Memory cycle writeback
    mem_write(state, state->g);
    // 10. RU WB
    state->b = u;
    // 11. RC WA
    state->a = state->b ^ 0177777;
}

static void exec_WAND0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RA WB
    state->b = state->a;
    // 3. RC WY
    uint16_t u = state->b ^ 0177777;
    // 4. RCH WB
    state->b = control_rch(state);
    // 5. RC RU WA
    state->a = (state->b ^ 0177777) | u;
    // 6. RA WB
    state->b = state->a;
    // 7. RC WA WCH
    state->a = state->b ^ 0177777;
    control_wch(state, state->a);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
}

static void exec_QXCH0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RQ WB
    state->b = state->q;
    // Memory cycle completion
    state->g |= mem_read(state);
    // 5. RG WQ
    state->q = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // Memory cycle writeback
    mem_write(state, state->g);
}

static void exec_STD2(agc_state_t *state) {
    // 1. RZ WY12 CI
    uint16_t u = control_add(state->z & 07777, 0, 1);
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
    // Memory cycle writeback
    mem_write(state, state->g);
}
