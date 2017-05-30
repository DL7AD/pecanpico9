#include "ch.h"
#include "hal.h"
#include "morse.h"
#include "debug.h"

static uint8_t *buffer;
static uint32_t c;
#define ADDB(bit) { \
	buffer[c/8] |= ((bit & 0x1) << (c % 8)); \
	c++; \
}

void dah(void)
{
	ADDB(1);
	ADDB(1);
	ADDB(1);
	ADDB(0);
}
void dit(void)
{
	ADDB(1);
	ADDB(0);
}
void blank(uint32_t ticks) {
	for(uint32_t i=0; i<ticks; i++)
		ADDB(0);
}

void morse_encode_char(char letter)
{
	switch(letter) {
		case 'A':
			dit();
			dah();
		break;
		case 'B':
			dah();
			dit();
			dit();
			dit();
		break;
		case 'C':
			dah();
			dit();
			dah();
			dit();
		break;
		case 'D':
			dah();
			dit();
			dit();
		break;
		case 'E':
			dit();
		break;
		case 'F':
			dit();
			dit();
			dah();
			dit();
		break;
		case 'G':
			dah();
			dah();
			dit();
		break;
		case 'H':
			dit();
			dit();
			dit();
			dit();
		break;
		case 'I':
			dit();
			dit();
		break;
		case 'J':
			dit();
			dah();
			dah();
			dah();
		break;
		case 'K':
			dah();
			dit();
			dah();
		break;
		case 'L':
			dit();
			dah();
			dit();
			dit();
		break;
		case 'M':
			dah();
			dah();
		break;
		case 'N':
			dah();
			dit();
		break;
		case 'O':
			dah();
			dah();
			dah();
		break;
		case 'P':
			dit();
			dah();
			dah();
			dit();
		break;
		case 'Q':
			dah();
			dah();
			dit();
			dah();
		break;
		case 'R':
			dit();
			dah();
			dit();
		break;
		case 'S':
			dit();
			dit();
			dit();
		break;
		case 'T':
			dah();
		break;
		case 'U':
			dit();
			dit();
			dah();
		break;
		case 'V':
			dit();
			dit();
			dit();
			dah();
		break;
		case 'W':
			dit();
			dah();
			dah();
		break;
		case 'X':
			dah();
			dit();
			dit();
			dah();
		break;
		case 'Y':
			dah();
			dit();
			dah();
			dah();
		break;
		case 'Z':
			dah();
			dah();
			dit();
			dit();
		break;
		case '1':
			dit();
			dah();
			dah();
			dah();
			dah();
		break;
		case '2':
			dit();
			dit();
			dah();
			dah();
			dah();
		break;
		case '3':
			dit();
			dit();
			dit();
			dah();
			dah();
		break;
		case '4':
			dit();
			dit();
			dit();
			dit();
			dah();
		break;
		case '5':
			dit();
			dit();
			dit();
			dit();
			dit();
		break;
		case '6':
			dah();
			dit();
			dit();
			dit();
			dit();
		break;
		case '7':
			dah();
			dah();
			dit();
			dit();
			dit();
		break;
		case '8':
			dah();
			dah();
			dah();
			dit();
			dit();
		break;
		case '9':
			dah();
			dah();
			dah();
			dah();
			dit();
		break;
		case '0':
			dah();
			dah();
			dah();
			dah();
			dah();
		break;
		case ' ':
			blank(3);
		break;
		case '.':
			dit();
			dah();
			dit();
			dah();
			dit();
			dah();
		break;
	}
	blank(4);
}

uint32_t morse_encode(uint8_t* data, const char* letter)
{
	// Blanking bits TODO: Replace this
	for(uint32_t i=0; i<256; i++)
		data[i] = 0;

	// Encode morse
	buffer = data; // Buffer
	c = 0; // Bitlength
	for(uint32_t i=0; letter[i]!=0; i++)
		morse_encode_char(letter[i]);

	return c;
}

