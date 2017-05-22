/* trackuino copyright (C) 2010  EA5HAV Javi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "ch.h"
#include "hal.h"
#include "ax25.h"
#include "config.h"
#include "debug.h"
#include "aprs.h"

#define AX25_WRITE_BIT(data, size) { \
	data[size >> 3] |= (1 << (size & 7)); \
}
#define AX25_CLEAR_BIT(data, size) { \
	data[size >> 3] &= ~(1 << (size & 7)); \
}

static void update_crc(ax25_t *packet, char bit)
{
	packet->crc ^= bit;
	if(packet->crc & 1)
		packet->crc = (packet->crc >> 1) ^ 0x8408;  // X-modem CRC poly
	else
		packet->crc = packet->crc >> 1;
}

uint32_t lfsr;
uint8_t scramble_bit(uint8_t _in) {
	uint8_t x = (_in ^ (lfsr >> 16) ^ (lfsr >> 11)) & 1;
	lfsr = (lfsr << 1) | (x & 1);
	return x;
}

static void send_byte(ax25_t *packet, char byte)
{
	int i;
	for(i=0; i<8; i++) {
		update_crc(packet, (byte >> i) & 1);
		if((byte >> i) & 1) {
			// Next bit is a '1'
			if(packet->size >= packet->max_size * 8)  // Prevent buffer overrun
				return;

			AX25_WRITE_BIT(packet->data, packet->size);

			packet->size++;
			packet->ones_in_a_row++;
			if(packet->ones_in_a_row < 5)
				continue;
		}
		// Next bit is a '0' or a zero padding after 5 ones in a row
		if(packet->size >= packet->max_size * 8)    // Prevent buffer overrun
			return;

		AX25_CLEAR_BIT(packet->data, packet->size);

		packet->size++;
		packet->ones_in_a_row = 0;
	}
}

void ax25_send_byte(ax25_t *packet, char byte)
{
	send_byte(packet, byte);
}

void ax25_send_sync(ax25_t *packet)
{
	unsigned char byte = 0x00;
	int i;
	for(i=0; i<8; i++, packet->size++) {
		if(packet->size >= packet->max_size * 8)  // Prevent buffer overrun
			return;
		if((byte >> i) & 1)
			packet->data[packet->size >> 3] |= (1 << (packet->size & 7));
		else
			packet->data[packet->size >> 3] &= ~(1 << (packet->size & 7));
	}
}

void ax25_send_flag(ax25_t *packet)
{
  unsigned char byte = 0x7e;
  int i;
  for(i=0; i<8; i++, packet->size++) {
    if(packet->size >= packet->max_size * 8)  // Prevent buffer overrun
      return;
    if((byte >> i) & 1)
      packet->data[packet->size >> 3] |= (1 << (packet->size & 7));
    else
      packet->data[packet->size >> 3] &= ~(1 << (packet->size & 7));
  }
}

void ax25_send_string(ax25_t *packet, const char *string)
{
	int i;
	for(i=0; string[i]; i++) {
		ax25_send_byte(packet, string[i]);
	}
}

void ax25_send_header(ax25_t *packet, const char *callsign, uint8_t ssid, const char *path, uint16_t preamble)
{
	uint8_t i, j;
	uint8_t tmp[8];
	packet->size = 0;
	packet->ones_in_a_row = 0;
	packet->crc = 0xffff;

	// Send preamble ("a bunch of 0s")
	if(packet->mod == MOD_2GFSK) {
		preamble = preamble * 6 / 5;
	} else {
		preamble = preamble * 3 / 20;
	}
	for(i=0; i<preamble; i++)
	{
		ax25_send_sync(packet);
	}

	// Send flag
	for(uint8_t i=0; i<4; i++)
	{
		ax25_send_flag(packet);
	}

	ax25_send_path(packet, APRS_DEST_CALLSIGN, APRS_DEST_SSID, false);		// Destination callsign
	ax25_send_path(packet, callsign, ssid, path[0] == 0 || path == NULL);	// Source callsign

	// Parse path
	for(i=0, j=0; (path[i-1] != 0 || i == 0) && path != NULL; i++) {
		if(path[i] == ',' || path[i] == 0) { // Found block in path
			if(!j) // Block empty
				break;

			// Filter Path until '-'
			tmp[j] = 0;
			char p[8];
			uint8_t t;
			for(t=0; t<j && tmp[t] != '-'; t++)
				p[t] = tmp[t];
			p[t] = 0;

			// Filter TTL
			uint8_t s = ((tmp[t] == '-' ? tmp[++t] : tmp[--t])-48) & 0x7;

			if(s != 0)
				ax25_send_path(packet, p, s, path[i] == 0);
			j = 0;

		} else {
			tmp[j++] = path[i];
		}
	}

	// Control field: 3 = APRS-UI frame
	send_byte(packet, 0x03);

	// Protocol ID: 0xf0 = no layer 3 data
	send_byte(packet, 0xf0);
}

void ax25_send_path(ax25_t *packet, const char *callsign, uint8_t ssid, bool last)
{
	uint8_t j;

	// Transmit callsign
	for(j = 0; callsign[j]; j++) {
		send_byte(packet, callsign[j] << 1);
	}

	// Transmit pad
	for( ; j < 6; j++)
		send_byte(packet, ' ' << 1);

	// Transmit SSID. Termination signaled with last bit = 1
	send_byte(packet, ('0' + ssid) << 1 | (last & 0x1));
}

void ax25_send_footer(ax25_t *packet)
{
	// Save the crc so that it can be treated it atomically
	uint16_t final_crc = packet->crc;

	// Send CRC
	send_byte(packet, ~(final_crc & 0xff));
	final_crc >>= 8;
	send_byte(packet, ~(final_crc & 0xff));

	packet->crc = final_crc;

	// Signal the end of frame
	ax25_send_flag(packet);
}

/**
  * Scrambling for 2GFSK
  */
void scramble(ax25_t *packet) {
	if(packet->mod != MOD_2GFSK)
		return; // Scrambling not necessary

	// Scramble
	lfsr = 0;
	for(uint32_t i=0; i<packet->size; i++) {
		uint8_t bit = scramble_bit((packet->data[i >> 3] >> (i & 0x7)) & 0x1);
		if(bit) {
			AX25_WRITE_BIT(packet->data, i);
		} else {
			AX25_CLEAR_BIT(packet->data, i);
		}
	}
}

/**
  * NRZ-I tone encoding (0: bit change, 1: no bit change)
  */
void nrzi_encode(ax25_t *packet) {
	uint8_t ctone = 0;
	for(uint32_t i=0; i<packet->size; i++) {
		if(((packet->data[i >> 3] >> (i & 0x7)) & 0x1) == 0)
			ctone = !ctone;
		if(ctone) {
			AX25_WRITE_BIT(packet->data, i);
		} else {
			AX25_CLEAR_BIT(packet->data, i);
		}
	}
}

