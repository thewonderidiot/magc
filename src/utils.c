#include "utils.h"

uint8_t check_parity(uint16_t word) {
    // Determine the parity of the given 16-bit word
    uint8_t parity = 0;
    while (word) {
        parity = !parity;
        word = word & (word - 1);
    }
    return parity;
}
