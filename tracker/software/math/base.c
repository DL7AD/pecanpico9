/**
 * Base64 encoding implementation
 * @see http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
 *
 * basE91 encoding/decoding routines
 *
 * Copyright (c) 2000-2006 Joachim Henke
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  - Neither the name of Joachim Henke nor the names of his contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ch.h"
#include "hal.h"
#include "base.h"
#include "debug.h"

static char b64_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int b64_mod_table[] = {0, 2, 1};

const unsigned char b91_table[91] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '#', '$',
	'%', '&', '(', ')', '*', '+', ',', '.', '/', ':', ';', '<', '=',
	'>', '?', '@', '[', ']', '^', '_', '`', '{', '|', '}', '~', '"'
};

typedef struct {
	uint64_t queue;
	uint64_t nbits;
	uint32_t val;
} base91_t;

void base64_encode(const uint8_t *in, uint8_t *out, uint16_t input_length) {
	uint32_t i,j;
	for(i=0, j=0; i<input_length;) {
		uint32_t octet_a = i < input_length ? (unsigned char)in[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)in[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)in[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		out[j++] = b64_table[(triple >> 3 * 6) & 0x3F];
		out[j++] = b64_table[(triple >> 2 * 6) & 0x3F];
		out[j++] = b64_table[(triple >> 1 * 6) & 0x3F];
		out[j++] = b64_table[(triple >> 0 * 6) & 0x3F];
	}

	for(i=0; i<(uint32_t)b64_mod_table[input_length % 3]; i++)
		out[BASE64LEN(input_length) - 1 - i] = '=';

	out[BASE64LEN(input_length)] = '\0';
}




void basE91_init(base91_t *b)
{
	b->queue = 0;
	b->nbits = 0;
	b->val = -1;
}

size_t basE91_encode(base91_t *b, const void *i, size_t len, void *o)
{
	const uint8_t *ib = i;
	uint8_t *ob = o;
	size_t n = 0;

	while (len--) {
		b->queue |= *ib++ << b->nbits;
		b->nbits += 8;
		if (b->nbits > 13) {	/* enough bits in queue */
			uint32_t val = b->queue & 8191;

			if (val > 88) {
				b->queue >>= 13;
				b->nbits -= 13;
			} else {	/* we can take 14 bits */
				val = b->queue & 16383;
				b->queue >>= 14;
				b->nbits -= 14;
			}
			ob[n++] = b91_table[val % 91];
			ob[n++] = b91_table[val / 91];
		}
	}

	return n;
}

size_t basE91_encode_end(base91_t *b, void *o)
{
	uint8_t *ob = o;
	size_t n = 0;

	if (b->nbits) {
		ob[n++] = b91_table[b->queue % 91];
		if (b->nbits > 7 || b->queue > 90)
			ob[n++] = b91_table[b->queue / 91];
	}
	b->queue = 0;
	b->nbits = 0;
	b->val = -1;

	return n;
}

void base91_encode(const uint8_t *in, uint8_t *out, uint16_t input_length) {
	base91_t handle;

	uint32_t ototal = 0;
	for(uint32_t i=0; i<(uint32_t)BASE91LEN(input_length); i++)
		out[i] = 0;

	basE91_init(&handle);
	ototal += basE91_encode(&handle, in, input_length, out);
	ototal += basE91_encode_end(&handle, out + ototal);
}









