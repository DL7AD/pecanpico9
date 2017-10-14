#ifndef __MORSE_H__
#define __MORSE_H__

#include "ch.h"
#include "hal.h"

uint32_t morse_encode(uint8_t* buffer, uint32_t length, const char* in);

#endif

