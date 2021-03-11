#include <string.h>
#include "agc.h"

int main(int argc, char **argv) {
    agc_state_t agc_state;
    memset(&agc_state, 0, sizeof(agc_state));

    agc_init(&agc_state);
    agc_state.f[04000] = 000004;
    agc_state.f[04001] = 034054;
    agc_state.f[04002] = 056006;
    agc_state.f[04054] = 012345;
    for (int i = 0; i < 10; i++) {
        agc_service(&agc_state);
        agc_service(&agc_state);
        agc_service(&agc_state);
        agc_service(&agc_state);
        agc_service(&agc_state);
    }
    return 0;
}
