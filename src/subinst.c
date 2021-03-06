//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include <stdio.h>
#include "control.h"
#include "mem.h"
#include "subinst.h"

//---------------------------------------------------------------------------//
//                         Local Function Prototypes                         //
//---------------------------------------------------------------------------//
#define X(_n,...) \
static void exec_##_n(agc_state_t *state);
SUBINSTRUCTIONS
#undef X

static void exec_DV1376(agc_state_t *state, uint8_t stage);

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
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
        break;
    SUBINSTRUCTIONS
#undef X
    default:
        printf("UNKNOWN INSTRUCTION\n");
    }

    if ((subinst_id != SUBINST_TC0) && (subinst_id != SUBINST_TCF0)) {
        state->only_tc = 0;
    }
}

void exec_PINC(agc_state_t *state, uint16_t rsct) {
    // 1. RSCT WS
    state->s = rsct;
    // 2. RSC WG (no effect)
    // STBE/STBF
    state->g = mem_read(state);
    // 5. RG WY TSGN TMZ TPZG
    // 6. PONEX
    uint16_t u = control_add(1, state->g, 0);
    // 7. RU WSC WG WOVR
    state->g = u;
    control_wovr(state, u);
    // 8. RB WS
    state->s = state->b & 07777;
    // ZID
    mem_write(state, state->g);
}

void exec_DINC(agc_state_t *state, uint16_t rsct) {
    // 1. RSCT WS
    state->s = rsct;
    // 2. RSC WG (no effect)
    // STBE/STBF
    state->g = mem_read(state);
    // 5. RG WY TSGN TMZ TPZG
    // 6. 00 MONEX POUT
    // 6. 10 PONEX MOUT
    // 6. X1 ZOUT
    uint16_t x = 0;
    if (state->g == 0 || state->g == 0177777) {
        control_zout(state);
    } else if (state->g & 0100000) {
        x = 1;
        control_mout(state);
    } else {
        x = 0177776;
        control_pout(state);
    }
    uint16_t u = control_add(x, state->g, 0);
    // 7. RU WSC WG WOVR
    state->g = u;
    // 8. RB WS
    state->s = state->b & 07777;
    // ZID
    mem_write(state, state->g);
}

//---------------------------------------------------------------------------//
//                        Local Function Definitions                         //
//---------------------------------------------------------------------------//
static void exec_TC0(agc_state_t *state) {
    // Reset TC Trap
    state->no_tc = 0;
    // 1. RB WY12 CI
    uint16_t u = control_add(state->b & 07777, 0, 1);
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // 3. RZ WQ
    state->q = state->z;
    // STBE/STBF
    state->g |= mem_read(state);
    // 6. RU WZ
    state->z = u;
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
    // ZID
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
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
    // 11. RU WA
    state->a = u;
}

static void exec_TCF0(agc_state_t *state) {
    // Reset TC Trap
    state->no_tc = 0;
    // 1. RB WY12 CI
    uint16_t u = control_add(state->b & 07777, 0, 1);
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // STBE/STBF
    state->g |= mem_read(state);
    // 6. RU WZ
    state->z = u;
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
    // ZID
    mem_write(state, state->g);
}

static void exec_DAS0(agc_state_t *state) {
    // 1. RL10BB WS WY12 MONEX CI
    state->s = state->b & 01777;
    uint16_t u = control_add(state->s, 0177776, 1);
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB
    state->b = state->a;
    // 4. RL WA
    state->a = (state->l & 0137777) | ((state->l >> 1) & 040000);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RU WL
    state->l = u;
    // 6. RG WY A2X
    u = control_add(state->g, state->a, 0);
    // 7. RB WA
    state->a = state->b;
    // 8. RL WB
    state->b = (state->l & 0137777) | ((state->l >> 1) & 040000);
    // 9. RU WSC WG TOV
    control_wsc(state, u);
    control_wg(state, u);
    uint16_t sign_bits = u & 0140000;
    uint16_t x = 0;
    if (sign_bits == 0040000) {
        x = 1;
    } else if (sign_bits == 0100000) {
        x = 0177776;
    }
    // 10. 00 RA WY ST1
    // 10. 01 RA WY ST1 PONEX
    // 10. 10 RA WY ST1 MONEX
    // 10. 11 RA WY ST1
    state->u = control_add(state->a, x, 0);
    state->st_pend = 1;
    // ZID
    mem_write(state, state->g);
}

static void exec_DAS1(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RU WA
    state->a = state->u;
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WY A2X
    uint16_t u = control_add(state->g, state->a, 0);
    // 6. RU WG WSC TOV
    control_wsc(state, u);
    control_wg(state, u);
    // 7. 00 WA
    // 7. 01 WA RB1
    // 7. 10 WA R1C
    // 7. 11 WA
    uint16_t sign_bits = u & 0140000;
    if (sign_bits == 0040000) {
        state->a = 1;
    } else if (sign_bits == 0100000) {
        state->a = 0177776;
    } else {
        state->a = 0;
    }
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RC TMZ
    // 10. X0 WL
    // 11. X1 RU WA
    if (state->b != 0) {
        state->l = 0;
    } else {
        state->a = u;
    }
    // ZID
    mem_write(state, state->g);
}

static void exec_LXCH0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RL WB
    state->b = (state->l & 0137777) | ((state->l >> 1) & 040000);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WL
    state->l = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // ZID
    mem_write(state, state->g);
}

static void exec_INCR0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WY TSGN TMZ TPZG
    // 6. PONEX
    uint16_t u = control_add(state->g, 1, 0);
    // 7. RU WSC WG WOVR
    control_wsc(state, u);
    control_wg(state, u);
    control_wovr(state, u);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // ZID
    mem_write(state, state->g);
}

static void exec_ADS0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
}

static void exec_CA0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
}

static void exec_CS0(agc_state_t *state) {
    // Transient on TC0
    state->no_tc = 0;
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
}

static void exec_NDX0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
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
    // ZID
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
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
}

static void exec_RSM3(agc_state_t *state) {
    // 1. R15 WS
    state->s = 015;
    // 2. RSC WG NISQ
    state->nisql = 1;
    // STBE/STBF
    state->g = mem_read(state);
    // 5. RG WZ
    state->z = state->g;
    state->iip = 0;
    state->only_rupt = 0;
    // 6. RB WG
    state->g = state->b;
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
    // ZID
    mem_write(state, state->g);
}

static void exec_DXCH0(agc_state_t *state) {
    // Transient on TCF0
    state->no_tc = 0;
    // 1. RL10BB WS WY12 MONEX CI
    state->s = state->b & 01777;
    uint16_t u = control_add(state->s, 0177776, 1);
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RL WB
    state->b = (state->l & 0137777) | ((state->l >> 1) & 040000);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WL
    state->l = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RU WS WB
    state->s = u;
    state->b = u;
    // ZID
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
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WA
    state->a = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // ZID
    mem_write(state, state->g);
}

static void exec_TS0(agc_state_t *state) {
    // Transient on TCF0
    state->no_tc = 0;
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB TOV
    state->b = state->a;
    uint16_t sign = state->b & 0140000;
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
}

static void exec_XCH0(agc_state_t *state) {
    // Transient on TCF0
    state->no_tc = 0;
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB
    state->b = state->a;
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WA
    state->a = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // ZID
    mem_write(state, state->g);
}

static void exec_AD0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RB WG
    control_wg(state, state->b);
    // ZID
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
    // STBE/STBF
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RC RA WY
    uint16_t u = (state->b ^ 0177777) | state->a;
    // ZID
    mem_write(state, state->g);
    // 10. RU WB
    state->b = u;
    // 11. RC WA
    state->a = state->b ^ 0177777;
}

static void exec_READ0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RA WB
    state->b = state->a;
    // 3. WY (no effect)
    // 4. RCH WB
    state->b = control_rch(state);
    // 5. RB WA
    state->a = state->b;
    // 6. RA WB (no effect)
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
}

static void exec_WRITE0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RA WB WG
    state->b = state->a;
    state->g = state->a;
    // 3. WY (no effect)
    // 4. RCH WB
    state->b = control_rch(state);
    // 5. RA WCH
    control_wch(state, state->a);
    // 6. RA WB
    state->b = state->a;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
}

static void exec_RAND0(agc_state_t *state) {
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
    // 7. RC WA
    state->a = state->b ^ 0177777;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
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

static void exec_ROR0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RA WB
    state->b = state->a;
    // 3. RB WY
    uint16_t u = state->b;
    // 4. RCH WB
    state->b = control_rch(state);
    // 5. RB RU WA
    state->a = state->b | u;
    // 6. RA WB
    state->b = state->a;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
}

static void exec_WOR0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RA WB
    state->b = state->a;
    // 3. RB WY
    uint16_t u = state->b;
    // 4. RCH WB
    state->b = control_rch(state);
    // 5. RB RU WA WCH
    state->a = state->b | u;
    control_wch(state, state->a);
    // 6. RA WB
    state->b = state->a;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
}

static void exec_RXOR0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RA WB
    state->b = state->a;
    // 3. RC RCH WY
    uint16_t ch = control_rch(state);
    uint16_t u = (state->b ^ 0177777) | ch;
    // 4. RCH WB
    state->b = ch;
    // 5. RA RC WG
    state->g = state->a | (state->b ^ 0177777);
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RC WG
    state->g = state->b ^ 0177777;
    // 10. RU WB
    state->b = u;
    // 11. RC RG WA
    state->a = (state->b ^ 0177777) | state->g;
}

static void exec_RUPT0(agc_state_t *state) {
    // 1. R15 WS
    state->s = 015;
    // 2. RSC WG (no effect)
    // STBE/STBF
    mem_read(state);
    // 9. RZ WG
    state->g = state->z;
    // 10. ST1
    state->st_pend = 1;
    // ZID
    mem_write(state, state->g);
}

static void exec_RUPT1(agc_state_t *state) {
    uint8_t rupt_num = 0;
    // 1. R15 RB2 WS
    state->s = 017;
    // 2. RSC WG (no effect)
    // 3. RRPA WZ
    if (state->pending_rupts) {
        rupt_num = __builtin_ffs(state->pending_rupts) - 1;
        state->z = 04000 + 4*rupt_num;
    } else {
        state->z = 00000;
    }
    // STBE/STBF
    mem_read(state);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RB WG KRPT
    state->g = state->b;
    if (rupt_num) {
        state->pending_rupts &= ~(1 << rupt_num);
    }
    state->iip = 1;
    state->no_rupt = 0;
    // ZID
    mem_write(state, state->g);
}

static void exec_DV0(agc_state_t *state) {
    // 1. RA WB TSGN TMZ
    state->b = state->a;
    uint16_t br1 = state->a & 0100000;
    uint16_t br2 = (state->a == 0177777);
    // 2. 0X RC WA TMZ DVST
    // 2. 1X DVST
    if ((state->a & 0100000) == 0) {
        state->a = state->b ^ 0177777;
        br2 = (state->a == 0177777);
    }
    // 3. RU WB STAGE
    state->st_pend = 1;
    // ------------------------- DV1 -------------------------
    // 4. X0 RL WB
    // 4. X1 RL WB TSGN
    state->b = (state->l & 0137777) | ((state->l >> 1) & 040000);
    if (br2) {
        br1 = state->l & 0100000;
    }
    // STBE/STBF
    state->g = mem_read(state);
    // 5. 0X RB WY B15X
    // 5. 1X RC WY B15X Z16
    uint16_t u;
    if (br1) {
        u = control_add(state->b ^ 0177777, 040000, 0);
        state->z |= 0100000;
    } else {
        u = control_add(state->b, 040000, 0);
    }
    // 6. RU WL TOV
    state->l = u;
    br2 = (u & 0140000) == 0040000;
    // 7. RG RSC WB TSGN
    state->b = state->g | control_rsc(state);
    br1 = state->b & 0100000;
    // 8. X0 RA WY PONEX
    // 8. X1 RA WY
    uint16_t x = 0;
    if (!br2) {
        x = 1;
    }
    u = control_add(state->a, x, 0);
    // 9. 0X RB WA
    // 9. 1X RC WA Z15
    if (br1) {
        state->a = state->b ^ 0177777;
        state->z |= 040000;
    } else {
        state->a = state->b;
    }
    // 10. RU WB
    state->b = u;
    // ZID
    mem_write(state, state->g);
    // 11. RL WYD
    uint16_t y = (state->l & 0100000) | ((state->l << 1) & 077776) | (state->l >> 15);
    // 12. RU WL
    state->l = y;
}

static void exec_DV1(agc_state_t *state) {
    exec_DV1376(state, 3);
}

static void exec_DV3(agc_state_t *state) {
    exec_DV1376(state, 7);
}

static void exec_DV7(agc_state_t *state) {
    exec_DV1376(state, 6);
}

static void exec_DV1376(agc_state_t *state, uint8_t stage) {
    uint16_t y;
    uint16_t u;
    // 1. L2GD RB WYD A2X PIFL
    state->g = (state->l & 0100000) | ((state->l << 1) & 077776);
    y = (state->b & 0100000) | ((state->b << 1) & 077776);
    if ((state->l & 040000) == 0) {
        y |= state->b >> 15;
    }
    u = control_add(state->a, y, 0);
    // 2. 0X RG WL TSGU DVST CLXC
    // 2. 1X RG WL TSGU DVST RB1F
    state->l = state->g;
    if (u & 0100000) {
        state->l |= 1;
    } else {
        u = y;
    }
    // 3. RU WB STAGE
    state->b = u;
    state->st_pend = stage;
    // ------------------------- NEXT STAGE -------------------------
    // 4. L2GD RB WYD A2X PIFL
    state->g = (state->l & 0100000) | ((state->l << 1) & 077776);
    y = (state->b & 0100000) | ((state->b << 1) & 077776);
    if ((state->l & 040000) == 0) {
        y |= state->b >> 15;
    }
    u = control_add(state->a, y, 0);
    // 5. 0X RG WL TSGU CLXC
    // 5. 1X RG WL TSGU RB1F
    state->l = state->g;
    if (u & 0100000) {
        state->l |= 1;
    } else {
        u = y;
    }
    // 6. RU WB
    state->b = u;
    // 7. L2GD RB WYD A2X PIFL
    state->g = (state->l & 0100000) | ((state->l << 1) & 077776);
    y = (state->b & 0100000) | ((state->b << 1) & 077776);
    if ((state->l & 040000) == 0) {
        y |= state->b >> 15;
    }
    u = control_add(state->a, y, 0);
    // 8. 0X RG WL TSGU CLXC
    // 8. 1X RG WL TSGU RB1F
    state->l = state->g;
    if (u & 0100000) {
        state->l |= 1;
    } else {
        u = y;
    }
    // 9. RU WB
    state->b = u;
    // 10. L2GD RB WYD A2X PIFL
    state->g = (state->l & 0100000) | ((state->l << 1) & 077776);
    y = (state->b & 0100000) | ((state->b << 1) & 077776);
    if ((state->l & 040000) == 0) {
        y |= state->b >> 15;
    }
    u = control_add(state->a, y, 0);
    // 11. 0X RG WL TSGU CLXC
    // 11. 1X RG WL TSGU RB1F
    state->l = state->g;
    if (u & 0100000) {
        state->l |= 1;
    } else {
        u = y;
    }
    // 12. RU WB
    state->b = u;
}

static void exec_DV6(agc_state_t *state) {
    uint16_t y;
    uint16_t u;
    // 1. L2GD RB WYD A2X PIFL
    state->g = (state->l & 0100000) | ((state->l << 1) & 077776);
    y = (state->b & 0100000) | ((state->b << 1) & 077776);
    if ((state->l & 040000) == 0) {
        y |= state->b >> 15;
    }
    u = control_add(state->a, y, 0);
    // 2. 0X RG WL TSGU DVST CLXC
    // 2. 1X RG WL TSGU DVST RB1F
    state->l = state->g;
    if (u & 0100000) {
        state->l |= 1;
    } else {
        u = y;
    }
    // 3. RU WB STAGE
    state->b = u;
    state->st_pend = 4;
    // ------------------------- DV4 -------------------------
    // 4. L2GD RB WYD A2X PIFL
    state->g = (state->l & 0100000) | ((state->l << 1) & 077776);
    y = (state->b & 0100000) | ((state->b << 1) & 077776);
    if ((state->l & 040000) == 0) {
        y |= state->b >> 15;
    }
    u = control_add(state->a, y, 0);
    // 5. 0X RG WB WA TSGU DVST CLXC
    // 5. 1X RG WB WA TSGU DVST RB1F
    state->b = state->g;
    state->a = state->g;
    if (u & 0100000) {
        state->b |= 1;
        state->a |= 1;
    } else {
        u = y;
    }
    // 6. RZ TOV
    uint16_t sign = (state->z & 0140000);
    // 7. 01 RC WA
    // 7. 1X RC WA
    if ((sign == 0100000) || (sign == 0040000)) {
        state->a = state->b ^ 0177777;
    }
    // 8. RZ WS ST2 TSGN RSTSTG
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RU WB WL
    state->b = u;
    state->l = u;
    // 10. 0X RC WL
    if ((state->z & 0100000) == 0) {
        state->l = state->b ^ 0177777;
    }
}

static void exec_BZF0(agc_state_t *state) {
    // 1. RA WG TSGN TMZ
    // 2. TPZG
    // 3. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    if (state->a == 0 || state->a == 0177777) {
        // 5. X1 RB WY12 CI
        uint16_t u = control_add(state->b & 07777, 0, 1);
        // 6. X1 RU WZ
        state->z = u;
        // 8. X1 RAD WB WS NISQ
        state->b = control_rad(state);
        state->s = state->b & 07777;
        state->nisql = 1;
        // Transient on TCF0 if not to EXTEND
        if (!state->futext) {
            state->no_tc = 0;
        }
    } else {
        // 8. X0 RZ WS ST2
        state->s = state->z & 07777;
        state->st_pend = 2;
    }
    // ZID
    mem_write(state, state->g);
}

static void exec_MSU0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WB
    state->b = state->g;
    // 6. RC WY CI A2X
    uint16_t u = control_add(state->b ^ 0177777, state->a, 1);
    // 7. RUS WA TSGN
    state->a = ((u << 1) & 0100000) | (u & 077777);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. 1X RA WY MONEX
    if (state->a & 0100000) {
        u = control_add(state->a, 0177776, 0);
    }
    // ZID
    mem_write(state, state->g);
    // 11. RUS WA
    state->a = ((u << 1) & 0100000) | (u & 077777);
}

static void exec_QXCH0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RQ WB
    state->b = state->q;
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WQ
    state->q = state->g;
    // 7. RB WSC WG
    control_wsc(state, state->b);
    control_wg(state, state->b);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // ZID
    mem_write(state, state->g);
}

static void exec_AUG0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WY TSGN TMZ TPZG
    // 6. 0X PONEX
    // 6. 1X MONEX
    uint16_t x = 1;
    if (state->g & 0100000) {
        x = 0177776;
    }
    uint16_t u = control_add(state->g, x, 0);
    // 7. RU WSC WG WOVR
    control_wsc(state, u);
    control_wg(state, u);
    control_wovr(state, u);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // ZID
    mem_write(state, state->g);
}

static void exec_DIM0(agc_state_t *state) {
    // 1. RL10BB WS
    state->s = state->b & 01777;
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RG WY TSGN TMZ TPZG
    // 6. 00 MONEX
    // 6. 10 PONEX
    uint16_t x;
    if (state->g == 0 || state->g == 0177777) {
        x = 0;
    } else if (state->g & 0100000) {
        x = 1;
    } else {
        x = 0177776;
    }
    uint16_t u = control_add(state->g, x, 0);
    // 7. RU WSC WG WOVR
    control_wsc(state, u);
    control_wg(state, u);
    control_wovr(state, u);
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // ZID
    mem_write(state, state->g);
}

static void exec_DCA0(agc_state_t *state) {
    // 1. RB WY12 MONEX CI
    uint16_t u = control_add(state->b, 0177776, 1);
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RU WS
    state->s = u & 07777;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. RB WL ST1
    state->l = state->b;
    state->st_pend = 1;
    // ZID
    mem_write(state, state->g);
}

static void exec_DCA1(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
}

static void exec_DCS0(agc_state_t *state) {
    // 1. RB WY12 MONEX CI
    uint16_t u = control_add(state->b, 0177776, 1);
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RU WS
    state->s = u & 07777;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. RC WL ST1
    state->l = state->b ^ 0177777;
    state->st_pend = 1;
    // ZID
    mem_write(state, state->g);
}

static void exec_DCS1(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
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
    // ZID
    mem_write(state, state->g);
}

static void exec_NDXX0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS
    state->s = state->z & 07777;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. ST1
    state->st_pend = 1;
    // ZID
    mem_write(state, state->g);
}

static void exec_NDXX1(agc_state_t *state) {
    // 1. RZ WY12 CI
    uint16_t u = (state->z & 07777) + 1;
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // 3. RB WZ
    state->z = state->b;
    // 4. RA WB
    state->b = state->a;
    // STBE/STBF
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
    // 10. RU WB EXT
    state->b = u;
    state->futext = 1;
    // ZID
    mem_write(state, state->g);
}

static void exec_SU0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 7. RG WB
    state->b = state->g;
    // 8. RZ WS ST2
    state->s = state->z & 07777;
    state->st_pend = 2;
    // 9. RB WG
    control_wg(state, state->b);
    // 10. RC WY A2X
    uint16_t u = control_add(state->b ^ 0177777, state->a, 0);
    // ZID
    mem_write(state, state->g);
    // 11. RU WA
    state->a = u;
}

static void exec_BZMF0(agc_state_t *state) {
    // 1. RA WG TSGN TMZ
    // 2. TPZG
    // 3. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    if (state->a == 0 || state->a == 0177777 || state->a & 0100000) {
        // 5. 01 RB WY12 CI
        // 5. 10 RB WY12 CI
        // 5. 11 RB WY12 CI
        uint16_t u = control_add(state->b & 07777, 0, 1);
        // 6. 01 RU WZ
        // 6. 10 RU WZ
        // 6. 11 RU WZ
        state->z = u;
        // 8. 01 RAD WB WS NISQ
        // 8. 10 RAD WB WS NISQ
        // 8. 11 RAD WB WS NISQ
        state->b = control_rad(state);
        state->s = state->b & 07777;
        state->nisql = 1;
        // Transient on TCF0 if to RELINT or INHINT
        if (state->pseudo && !state->futext) {
            state->no_tc = 0;
        }
    } else {
        // 8. 00 RZ WS ST2
        state->s = state->z & 07777;
        state->st_pend = 2;
    }
    // ZID
    mem_write(state, state->g);
}

static void exec_MP0(agc_state_t *state) {
    // 2. RSC WG
    state->g = control_rsc(state);
    // 3. RA WB TSGN
    state->b = state->a;
    // 4. 0X RB WL
    // 4. 1X RC WL
    state->l = state->b;
    uint16_t br1 = state->b & 0100000;
    if (br1) {
        state->l ^= 0177777;
    }
    // STBE/STBF
    state->g |= mem_read(state);
    // 7. RG WB TSGN2
    state->b = state->g;
    uint16_t br2 = state->b & 0100000;
    // 8. RZ WS
    state->s = state->z & 07777;
    // 9. 00 RB WY
    // 9. 01 RB WY CI
    // 9. 10 RC WY CI
    // 9. 11 RC WY
    uint16_t y = state->b;
    uint16_t ci = 0;
    if (br1) {
        y ^= 0177777;
    }
    if (br1 != br2) {
        ci = 1;
    }
    uint16_t u = control_add(y, 0, ci);
    // 10. RU WB TSGN ST1 NEACON
    state->b = u;
    state->st_pend = 1;
    // ZID
    mem_write(state, state->g);
    // 11. 0X WA
    // 11. 1X WA RB1 R1C L16
    if (u & 0100000) {
        state->a = 0177777;
        state->l |= 0100000;
    } else {
        state->a = 0;
    }
}

static void exec_MP1(agc_state_t *state) {
    // 1. ZIP
    control_zip(state);
    // 2. ZAP
    control_zap(state);
    // 3. ZIP
    control_zip(state);
    // 4. ZAP
    control_zap(state);
    // 5. ZIP
    control_zip(state);
    // 6. ZAP
    control_zap(state);
    // 7. ZIP
    control_zip(state);
    // 8. ZAP
    control_zap(state);
    // 9. ZIP
    control_zip(state);
    // 10. ZAP ST1 ST2
    control_zap(state);
    state->st_pend = 3;
    // 11. ZIP
    control_zip(state);
}

static void exec_MP3(agc_state_t *state) {
    // 1. ZAP
    control_zap(state);
    // 2. ZIP NISQ
    control_zip(state);
    state->nisql = 1;
    // 3. ZAP
    control_zap(state);
    // 4. RSC WG
    state->g = control_rsc(state);
    // STBE/STBF
    state->g |= mem_read(state);
    // 5. RZ WY12 CI
    uint16_t u = control_add(state->z & 07777, 0, 1);
    // 6. RU WZ TL15 NEACOF
    state->z = u;
    uint16_t br1 = state->l & 040000;
    // 7. 1X RB WY A2X
    if (br1) {
        u = (state->b + state->a) & 0177777;
    }
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
    // 9. RA (no effect)
    // 10. RL (no effect)
    // ZID
    mem_write(state, state->g);
    // 11. 1X RU WA
    if (br1) {
        state->a = u;
    }
}

static void exec_STD2(agc_state_t *state) {
    // 1. RZ WY12 CI
    uint16_t u = control_add(state->z & 07777, 0, 1);
    // 2. RSC WG NISQ
    state->g = control_rsc(state);
    state->nisql = 1;
    // STBE/STBF
    state->g |= mem_read(state);
    // 6. RU WZ
    state->z = u;
    // 8. RAD WB WS
    state->b = control_rad(state);
    state->s = state->b & 07777;
    // ZID
    mem_write(state, state->g);
}
