//---------------------------------------------------------------------------//
//                                 Includes                                  //
//---------------------------------------------------------------------------//
#include "counter.h"
#include "control.h"

//---------------------------------------------------------------------------//
//                        Global Function Definitions                        //
//---------------------------------------------------------------------------//
void control_gojam(agc_state_t *state) {
    state->nisql = 0;
    state->futext = 0;
    state->sqext = 0;
    state->sq = 0;
    state->qc = 0;
    state->sqr10 = 0;
    state->st = 1;
    state->st_pend = 0;
    state->iip = 0;
    state->inhint = 0;
    state->gnhnc = 1;
    state->inkl = 0;
    state->restart = 1;
    state->dsky.restart = 1;
}

uint16_t control_add(uint16_t x, uint16_t y, uint16_t ci) {
    uint32_t u = (uint32_t)x + (uint32_t)y;
    u += ((u >> 16) & 1) | ci;
    return (u & 0177777);
}

uint16_t control_rad(agc_state_t *state) {
    switch (state->g) {
    case 000003:
        state->inhint = 0;
        state->st_pend = 2;
        state->pseudo = 1;
        return state->z;
    case 000004:
        state->inhint = 1;
        state->st_pend = 2;
        state->pseudo = 1;
        return state->z;
    case 000006:
        state->futext = 1;
        state->st_pend = 2;
        state->pseudo = 1;
        return state->z;
    default:
        state->pseudo = 0;
        return state->g;
    }
}

uint16_t control_rsc(agc_state_t *state) {
    switch (state->s) {
    case 0: 
        return state->a;
    case 1:
        return (state->l & 0137777) | ((state->l >> 1) & 040000);
    case 2:
        return state->q;
    case 3:
        return state->eb;
    case 4:
        return state->fb | ((state->fb << 1) & 0100000);
    case 5:
        return state->z;
    case 6:
        return (state->eb >> 8) | state->fb | ((state->fb << 1) & 0100000);
    case 7:
        return 0;
    case 020:
    case 021:
    case 022:
    case 023:
        state->edit = state->s;
        // fallthrough
    default:
        return 0;
    }
}

void control_wsc(agc_state_t *state, uint16_t wl) {
    switch (state->s) {
    case 0: 
        state->a = wl;
        break;
    case 1:
        state->l = wl;
        break;
    case 2:
        state->q = wl;
        break;
    case 3:
        state->eb = wl & 03400;
        break;
    case 4:
        state->fb = ((wl >> 1) & 040000) | (wl & 036000);
        break;
    case 5:
        state->z = wl;
        break;
    case 6:
        state->eb = (wl << 8) & 03400;
        state->fb = ((wl >> 1) & 040000) | (wl & 036000);
        break;
    default:
        break;
    }
}

void control_wg(agc_state_t *state, uint16_t wl) {
    switch (state->edit) {
    case 020:
        state->g = ((wl >> 2) & 020000) | ((wl >> 1) & 017777) | ((wl << 15) & 0100000);
        break;
    case 021:
        state->g = (wl & 0100000) | ((wl >> 2) & 020000) | ((wl >> 1) & 017777);
        break;
    case 022:
        state->g = ((wl << 2) & 0100000) | ((wl << 1) & 037776) | (wl >> 15);
        break;
    case 023:
        state->g = (wl >> 7) & 0177;
        break;
    default:
        state->g = wl;
        break;
    }
}

void control_zip(agc_state_t *state) {
    // A2X L2GD
    uint16_t y = 0;
    uint16_t ci = 0;
    uint16_t mcro = 0;
    switch (state->l & 040003) {
    case 040003:
        // MCRO
        mcro = 1;
        // fallthrough
    case 000000:
        // WY
        y = 0;
        break;
    case 000001:
    case 040000:
        // RB WY
        y = state->b;
        break;
    case 000002:
    case 040001:
        // RB WYD
        y = (state->b & 0100000) | ((state->b << 1) & 077776);
        break;
    case 000003:
    case 040002:
        // RC WY CI MCRO
        y = state->b ^ 0177777;
        ci = 1;
        mcro = 1;
        break;
    }

    state->u = (state->a + y + ci) & 0177777;
    state->g = (state->l & 0100000) | ((state->l << 1) & 077776) | mcro;
}

void control_zap(agc_state_t *state) {
    // RU G2LS WALS
    state->a = (state->u >> 2);
    uint16_t a_sign = state->g;
    if (state->g & 01) {
        a_sign = state->u;
    }
    state->a |= (a_sign & 0100000) | ((a_sign >> 1) & 040000);
    state->l = (state->g & 0100000) | ((state->g >> 3) & 07777) |
               ((state->g << 14) & 040000) | ((state->u << 12) & 030000);
}

void control_wovr(agc_state_t *state, uint16_t wl) {
    if ((wl & 0140000) == 0040000) {
        switch (state->s) {
        case 025:
            counter_request(state, COUNTER_TIME2, COUNT_UP);
            break;
        case 026:
            state->pending_rupts |= (1 << RUPT_T3RUPT);
            break;
        case 027:
            state->pending_rupts |= (1 << RUPT_T4RUPT);
            break;
        case 030:
            state->pending_rupts |= (1 << RUPT_T5RUPT);
            break;
        }
    }
}

uint16_t control_rch(agc_state_t *state) {
    uint16_t chan = state->s & 077;
    uint16_t chwl;
    switch (chan) {
    case 001:
        return (state->l & 0137777) | ((state->l >> 1) & 040000);
    case 002:
        return state->q;
    case 003:
        return (state->scaler >> 17) & 037777;
    case 004:
        return (state->scaler >> 3) & 037777;
    case 005:
        return state->chan5;
    case 006:
        return state->chan6;
    case 007:
        return (state->feb >> 9);
    case 010:
        chwl = state->dsky.out0;
        break;
    case 011:
        chwl = state->chan11;
        break;
    case 012:
        chwl = state->chan12;
        break;
    case 013:
        chwl = state->chan13;
        break;
    case 014:
        chwl = state->chan14;
        break;
    case 015:
        return state->chan15;
    case 016:
        return state->chan16;
    case 030:
    case 031:
    case 032:
    case 033:
        return 0177777;
    default:
        return 0;
    }

    return ((chwl << 1) & 0100000) | chwl;
}

void control_wch(agc_state_t *state, uint16_t wl) {
    uint16_t chan = state->s & 077;
    uint16_t chwl = ((wl >> 1) & 040000) | (wl & 037777);
    switch (chan) {
    case 001:
        state->l = wl;
        break;
    case 002:
        state->q = wl;
        break;
    case 005:
        state->chan5 = wl & 0377;
        break;
    case 006:
        state->chan6 = wl & 0377;
        break;
    case 007:
        state->feb = (wl << 9) & 0160000;
        break;
    case 010:
        state->dsky.out0 = chwl;
        break;
    case 011:
        state->chan11 = chwl;
        state->dsky.oper_err = (!state->flash && (chwl & 0100));
        state->dsky.vnflash =  (state->flash && (chwl & 040));
        state->dsky.key_rel =  (!state->flash && (chwl & 020));
        state->dsky.temp = (chwl & 010) != 0;
        state->dsky.upl_act = (chwl & 04) != 0;
        state->dsky.comp_act = (chwl & 02) != 0;
        if (chwl & 01000) {
            state->restart = 0;
            if (!state->altest) {
                state->dsky.restart = 0;
            }
        }
        break;
    case 012:
        state->chan12 = chwl;
        break;
    case 013:
        state->chan13 = chwl;
        state->altest = (chwl & 01000) != 0;
        state->dsky.restart = state->restart || state->altest;
        state->dsky.stby = state->stby || state->altest;
        break;
    case 014:
        state->chan14 = chwl;
        break;
    }
}
