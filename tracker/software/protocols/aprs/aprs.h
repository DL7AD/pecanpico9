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

#ifndef __APRS_H__
#define __APRS_H__

#include "config.h"
#include "si4464.h"
#include "tracking.h"
#include "ax25.h"

#define GSP_FIX_OLD						0x0
#define GSP_FIX_CURRENT					0x1

#define NMEA_SRC_OTHER					0x0
#define NMEA_SRC_GLL					0x1
#define NMEA_SRC_GGA					0x2
#define NMEA_SRC_RMC					0x3

#define ORIGIN_COMPRESSED				0x0
#define ORIGIN_TNC_BTEXT				0x1
#define ORIGIN_SOFTWARE					0x2
#define ORIGIN_RESERVED					0x3
#define ORIGIN_KPC3						0x4
#define ORIGIN_PICO						0x5
#define ORIGIN_OTHER_TRACKER			0x6
#define ORIGIN_DIGIPEATER_CONVERSION	0x7

#define APRS_DEST_CALLSIGN				"APECAN" // APExxx = Pecan device
#define APRS_DEST_SSID					0

#define SYM_BALLOON						0x2F4F
#define SYM_SMALLAIRCRAFT				0x2F27
#define SYM_SATELLITE					0x5C53
#define SYM_CAR                         0x2F3E
#define SYM_SHIP                        0x2F73

void aprs_encode_position(ax25_t* packet, const aprs_conf_t *config, trackPoint_t *trackPoint);
void aprs_encode_telemetry_configuration(ax25_t* packet, const aprs_conf_t *config, const telemetry_conf_t type);
void aprs_encode_message(ax25_t* packet, const aprs_conf_t *config, const char *receiver, const char *text);

void aprs_encode_init(ax25_t* packet, uint8_t* buffer, uint16_t size, mod_t mod);
void aprs_encode_data_packet(ax25_t* packet, char packetType, const aprs_conf_t *config, uint8_t *data, size_t size);
uint32_t aprs_encode_finalize(ax25_t* packet);

#endif

