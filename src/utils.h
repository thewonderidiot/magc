#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>

#define NUM_ELEMENTS(x) (sizeof((x))/sizeof((x)[0]))

uint8_t check_parity(uint16_t word);

#endif//_UTILS_H_
