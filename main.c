#include <string.h>
#include "agc.h"

int main(int argc, char **argv) {
    agc_state_t agc_state;
    memset(&agc_state, 0, sizeof(agc_state));

    if (argc < 2) {
        return 1;
    }

    agc_init(&agc_state);
    agc_load_rope(&agc_state, argv[1]);

    for (int i = 0; i < 10; i++) {
        agc_service(&agc_state);
    }
    return 0;
}
