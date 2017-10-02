#ifndef __BASE128_H__
#define __BASE128_H__

#include <stdint.h>

uint32_t b128_encode(uint8_t *infile, uint8_t *outfile, uint32_t input_length);
uint32_t b128_decode(uint8_t *infile, uint8_t *outfile, uint32_t input_length);

#endif

