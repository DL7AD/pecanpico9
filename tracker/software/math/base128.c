#include <stdint.h>

#define PADCHAR		'!'
#define INBYTES		7
#define OUTBYTES	8

#define ZOF_TAB		256
#define	BAD			0xFF
#define PAD 		0xFE	/* Encoded '!' is padding	*/

static const unsigned char	dtab[ZOF_TAB] = 
{
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, 187, BAD, 188, BAD, BAD, BAD, BAD,
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, 189, 190, BAD, BAD, BAD,
	191, PAD, BAD,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,
	 13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28, 
	 29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
	 45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56, BAD,  57,  58,  59,
	 60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,
	 76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, BAD,
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,
	 91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
	107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 
	123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
	139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 
	171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
};
static const char	*etab = "#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}~"
							"\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF"
							"\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF"
							"\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF"
							"\t\x0B\x1B\x1C ";

uint32_t b128_encode(uint8_t *infile, uint8_t *outfile, uint32_t input_length)
{
	unsigned char	in[INBYTES];
	unsigned char	topbits, mask;
	int 			i, len;
	uint32_t k = 0;
	uint32_t j = 0;

	while(k < input_length)
	{
		len = 0;
		for (i = 0; i < INBYTES; i++)
		{
			in[i] = infile[k++];
			len++;
			if (k >= input_length) {
				break;
			}
		}
		if (len > 0)
		{
			topbits = 0;
			mask = 0x40;
			for (i = 0; i < len; i++)
			{
				outfile[j++] = etab[in[i] & 0x7F];
				if (in[i] > 0x7F)
					topbits |= mask;
				mask >>= 1;
			}
			if (len < INBYTES)	/* Need to pad */
			{
				outfile[j++] = PADCHAR;
			}
			outfile[j++] = etab[topbits];
		}
	}
	outfile[j] = 0;
	return j;
}

uint32_t b128_decode(uint8_t *infile, uint8_t *outfile, uint32_t input_length)
{
	unsigned char 	in[OUTBYTES];
	unsigned char 	topbits, mask;
	int 			i, len;
	uint32_t k = 0;
	uint32_t j = 0;

	while(k < input_length)
	{
		len = 0;
		for (i = 0; i < OUTBYTES; i++)
		{
			in[i] = infile[k++];
			len++;
			if (k >= input_length)
				break;
		}
		if (len > 0)
		{
			topbits = dtab[in[len - 1]];
			mask = 0x40;
			for (len = i = 0; i < INBYTES; i++)
			{
				if (in[i] == PADCHAR)
					break;
				len++;
				in[i] = dtab[in[i]];
				if (in[i] == BAD)		/* Illegal encoding character, bail out */
					return 0;
				if (topbits & mask)
					in[i] |= 0x80;
				mask >>= 1;
			}
			for (i = 0; i < len; i++)
			{
				outfile[j++] = in[i];
			}
		}
	}
	return j;
}

