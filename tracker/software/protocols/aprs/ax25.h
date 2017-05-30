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

#ifndef __AX25_H__
#define __AX25_H__

#include "ch.h"
#include "hal.h"
#include "si4464.h"

typedef struct {
	char callsign[7];
	unsigned char ssid;
} address_t;

typedef struct {
	uint8_t ones_in_a_row;	// Ones in a row (for bitstuffing)
	uint8_t *data;			// Data
	uint16_t size;			// Packet size in bits
	uint16_t max_size;		// Max. Packet size in bits (size of modem packet)
	uint16_t crc;			// CRC
	mod_t mod;				// Modulation type (MOD_AFSK or MOD_2GFSK)
} ax25_t;

void ax25_send_header(ax25_t *packet, const char *callsign, uint8_t ssid, const char *path, uint16_t preamble);
void ax25_send_path(ax25_t *packet, const char *callsign, uint8_t ssid, bool last);
void ax25_send_byte(ax25_t *packet, char byte);
void ax25_send_string(ax25_t *packet, const char *string);
void ax25_send_footer(ax25_t *packet);
void scramble(ax25_t *packet);
void nrzi_encode(ax25_t *packet);

#endif

