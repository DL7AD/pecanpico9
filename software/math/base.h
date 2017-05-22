#ifndef __BASE_H__
#define __BASE_H__

#include "ch.h"
#include "hal.h"
#include <math.h>

#define BASE64LEN(in) (4 * (((in) + 2) / 3))
#define BASE91LEN(in) ((((in)*16)+26) / 13)

void base64_encode(const uint8_t *in, uint8_t *out, uint16_t input_length);
void base91_encode(const uint8_t *in, uint8_t *out, uint16_t input_length);

#endif
