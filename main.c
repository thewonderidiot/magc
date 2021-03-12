#include <string.h>
#include <stdio.h>
#include "agc.h"

int main(int argc, char **argv) {
    agc_state_t agc_state;
    memset(&agc_state, 0, sizeof(agc_state));

    if (argc < 2) {
        return 1;
    }

    agc_init(&agc_state);
    agc_load_rope(&agc_state, argv[1]);

    for (int i = 0; i < 300; i++) {
        agc_service(&agc_state);
        printf("A=%06o L=%06o Q=%06o Z=%06o FB=%02o EB=%o\n", agc_state.a, agc_state.l, agc_state.q, agc_state.z,
               agc_state.fb >> 10, agc_state.eb >> 8);
        printf("B=%06o G=%06o S=%04o EXT=%o INH=%o\n", agc_state.b, agc_state.g, agc_state.s, agc_state.sqext, agc_state.inhint);
    }
    return 0;
}
