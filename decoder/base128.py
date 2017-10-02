#  Copyright 2013-2016 Alex Danilo
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#	Base 128/192 encoding experiment.
#
#	Ths is just a quick experiment to see what sort of speed we get by encoding
#	a contiguous 128 value range in bytes that replaces base 64 with it's low level
#	conditionals and bit-bashing for decode.
#
#	Ultimately, the encoding I'd like to use is a 192 value encoding which yields
#	7.5 bits/byte - i.e. approx. 6.66% loss encoding binary data in a text-safe
#	transfer form. The 128 value coding is just to test the basis of the
#	speed and compressibility of the result and is easy to use in JS. The 192
#  value encoding is more efficient but likely a lot trickier to use from JS.
#
#	NB: This is using ISO-9959-1 as it's basis. The source code will break if you
#	try to view as UTF-8.
 

#	The decode table reverses the encoding back to bits. Note, that since the most we can encode is 7.5 bits, we can never
#	generate 0xFF in the decode so use that to mark 0xFF encoding bytes.

import struct

dtab = [
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 187, 0xFF, 188, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 189, 190, 0xFF, 0xFF, 0xFF,
	191, 0xFE, 0xFF,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,
	 13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28, 
	 29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
	 45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56, 0xFF,  57,  58,  59,
	 60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,
	 76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
	107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 
	123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
	139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 
	171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186
]

#	Reserve " (0x3E) as delimiter for easy manipulation in JS, '!' as stuffing character and generation for JS - the ranges 23->7E then A0->FF create the encoding.
#	That's enough for the base 128 version but we need 4 more characters to encode 7.5 bits/byte so the last set is carefully chosen from within
#	the control character range using values that are safe for editors.
etab = "#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}~\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF\t\x0B\x1B\x1C ";

INBYTES		= 7
OUTBYTES	= 8

def encode(infile):
	outfile = ''
	k = 0

	while k < len(infile):
		_len = 0
		_in = []
		for i in range(INBYTES):
			_in.append(infile[k])
			k += 1
			_len += 1
			if k >= len(infile):
				break

		if _len > 0:
			topbits = 0
			mask = 0x40
			for i in range(_len):
				outfile += etab[_in[i] & 0x7F]
				if _in[i] > 0x7F:
					topbits |= mask
				mask >>= 1

			if _len < INBYTES:
				outfile += '!'

			outfile += etab[topbits]

	return outfile

def decode(infile):
	out = bytearray()
	k = 0

	while k < len(infile):
		_len = 0
		_in = []
		for i in range(OUTBYTES):
			_in.append(ord(infile[k]))
			k += 1
			_len += 1;
			if k >= len(infile):
				break

		if _len > 0:
			topbits = dtab[_in[_len - 1]]
			mask = 0x40
			_len = 0
			for i in range(len(_in)-1):
				if _in[i] == 0x21:
					break
				_len += 1
				_in[i] = dtab[_in[i]]
				if _in[i] == 0xFF:
					return 0
				if topbits & mask:
					_in[i] |= 0x80
				mask >>= 1

			for i in range(_len):
				out += struct.pack('B', _in[i]) 

	return out

