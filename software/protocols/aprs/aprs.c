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
#include "config.h"
#include "ax25.h"
#include "aprs.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "base.h"
#include "max.h"
#include "debug.h"

#define METER_TO_FEET(m) (((m)*26876) / 8192)

static uint16_t loss_of_gps_counter = 0;
static uint16_t msg_id;

/**
 * Transmit APRS position packet. The comments are filled with:
 * - Static comment (can be set in config.h)
 * - Battery voltage in mV
 * - Solar voltage in mW (if tracker is solar-enabled)
 * - Temperature in Celcius
 * - Air pressure in Pascal
 * - Number of satellites being used
 * - Number of cycles where GPS has been lost (if applicable in cycle)
 */
uint32_t aprs_encode_position(uint8_t* message, mod_t mod, const aprs_config_t *config, trackPoint_t *trackPoint)
{
	char temp[22];
	ptime_t date = trackPoint->time;
	ax25_t packet;
	packet.data = message;
	packet.max_size = 512; // TODO: replace 512 with real size
	packet.mod = mod;

	ax25_send_header(&packet, config->callsign, config->ssid, config->path, config->preamble);
	ax25_send_byte(&packet, '/');                // Report w/ timestamp, no APRS messaging. $ = NMEA raw data

	// 170915 = 17h:09m:15s zulu (not allowed in Status Reports)
	chsnprintf(temp, sizeof(temp), "%02d%02d%02dh", date.hour, date.minute, date.second);
	ax25_send_string(&packet, temp);

	// Latitude
	uint32_t y = 380926 * (90 - trackPoint->gps_lat/10000000.0);
	uint32_t y3  = y   / 753571;
	uint32_t y3r = y   % 753571;
	uint32_t y2  = y3r / 8281;
	uint32_t y2r = y3r % 8281;
	uint32_t y1  = y2r / 91;
	uint32_t y1r = y2r % 91;

	// Longitude
	uint32_t x = 190463 * (180 + trackPoint->gps_lon/10000000.0);
	uint32_t x3  = x   / 753571;
	uint32_t x3r = x   % 753571;
	uint32_t x2  = x3r / 8281;
	uint32_t x2r = x3r % 8281;
	uint32_t x1  = x2r / 91;
	uint32_t x1r = x2r % 91;

	// Altitude
	uint32_t a = logf(METER_TO_FEET(trackPoint->gps_alt)) / logf(1.002f);
	uint32_t a1  = a / 91;
	uint32_t a1r = a % 91;

	uint8_t gpsFix = trackPoint->gps_lock ? GSP_FIX_CURRENT : GSP_FIX_OLD;
	uint8_t src = NMEA_SRC_GGA;
	uint8_t origin = ORIGIN_PICO;

	temp[0]  = (config->symbol >> 8) & 0xFF;
	temp[1]  = y3+33;
	temp[2]  = y2+33;
	temp[3]  = y1+33;
	temp[4]  = y1r+33;
	temp[5]  = x3+33;
	temp[6]  = x2+33;
	temp[7]  = x1+33;
	temp[8]  = x1r+33;
	temp[9]  = config->symbol & 0xFF;
	temp[10] = a1+33;
	temp[11] = a1r+33;
	temp[12] = ((gpsFix << 5) | (src << 3) | origin) + 33;
	temp[13] = 0;

	ax25_send_string(&packet, temp);

	// Comments
	ax25_send_string(&packet, "SATS ");
	chsnprintf(temp, sizeof(temp), "%d", trackPoint->gps_sats);
	ax25_send_string(&packet, temp);

	if(trackPoint->gps_lock)
	{
		// TTFF (Time to first fix)
		ax25_send_string(&packet, " TTFF ");
		chsnprintf(temp, sizeof(temp), "%d", trackPoint->gps_ttff);
		ax25_send_string(&packet, temp);
		ax25_send_string(&packet, "sec");
	}

	// GPS Loss counter
	if(!trackPoint->gps_lock)
	{
		ax25_send_string(&packet, " GPS LOSS ");
		chsnprintf(temp, sizeof(temp), "%d", ++loss_of_gps_counter);
		ax25_send_string(&packet, temp);
	} else {
		loss_of_gps_counter = 0;
	}

	temp[2] = 0;

	ax25_send_byte(&packet, '|');

	// Sequence ID
	uint32_t t = trackPoint->id & 0x1FFF;
	temp[0] = t/91 + 33;
	temp[1] = t%91 + 33;
	ax25_send_string(&packet, temp);

	// Telemetry parameter
	for(uint8_t i=0; i<5; i++) {
		switch(config->tel[i]) {
			case TEL_SATS:	t = trackPoint->gps_sats;			break;
			case TEL_TTFF:	t = trackPoint->gps_ttff;			break;
			case TEL_VBAT:	t = trackPoint->adc_vbat;			break;
			case TEL_VSOL:	t = trackPoint->adc_vsol;			break;
			case TEL_PBAT:	t = trackPoint->adc_pbat;			break;
			case TEL_PSOL:	t = trackPoint->adc_psol;			break;
			case TEL_HUM:	t = trackPoint->air_hum;			break;
			case TEL_PRESS:	t = trackPoint->air_press/125 - 40;	break;
			case TEL_TEMP:	t = trackPoint->air_temp/10 + 1000;	break;
		}

		temp[0] = t/91 + 33;
		temp[1] = t%91 + 33;
		ax25_send_string(&packet, temp);
	}

	ax25_send_byte(&packet, '|');

	ax25_send_footer(&packet);
	scramble(&packet);
	nrzi_encode(&packet);

	return packet.size;
}

/**
 * Transmit custom experimental packet
 */
uint32_t aprs_encode_experimental(char packetType, uint8_t* message, mod_t mod, const aprs_config_t *config, uint8_t *data, size_t size)
{
	ax25_t packet;
	packet.data = message;
	packet.max_size = 512; // TODO: replace 512 with real size
	packet.mod = mod;

	// Encode APRS header
	ax25_send_header(&packet, config->callsign, config->ssid, config->path, config->preamble);
	ax25_send_string(&packet, "{{");
	ax25_send_byte(&packet, packetType);

	// Encode message
	for(uint16_t i=0; i<size; i++)
		ax25_send_byte(&packet, data[i]);

	// Send footer
	ax25_send_footer(&packet);
	scramble(&packet);
	nrzi_encode(&packet);

	return packet.size;
}

/**
 * Transmit message packet
 */
uint32_t aprs_encode_message(uint8_t* message, mod_t mod, const aprs_config_t *config, const char *receiver, const char *text)
{
	ax25_t packet;
	packet.data = message;
	packet.max_size = 512; // TODO: replace 512 with real size
	packet.mod = mod;

	// Encode APRS header
	char temp[10];
	ax25_send_header(&packet, config->callsign, config->ssid, config->path, config->preamble);
	ax25_send_byte(&packet, ':');

	chsnprintf(temp, sizeof(temp), "%-9s", receiver);
	ax25_send_string(&packet, temp);

	ax25_send_byte(&packet, ':');
	ax25_send_string(&packet, text);
	ax25_send_byte(&packet, '{');

	chsnprintf(temp, sizeof(temp), "%d", ++msg_id);
	ax25_send_string(&packet, temp);

	// Send footer
	ax25_send_footer(&packet);
	scramble(&packet);
	nrzi_encode(&packet);

	return packet.size;
}

/**
 * Transmit APRS telemetry configuration
 */
uint32_t aprs_encode_telemetry_configuration(uint8_t* message, mod_t mod, const aprs_config_t *config, const telemetry_config_t type)
{
	char temp[4];
	ax25_t packet;
	packet.data = message;
	packet.max_size = 512; // TODO: replace 512 with real size
	packet.mod = mod;

	ax25_send_header(&packet, config->callsign, config->ssid, config->path, config->preamble); // Header
	ax25_send_byte(&packet, ':'); // Message flag

	// Callsign
	ax25_send_string(&packet, config->callsign);
	ax25_send_byte(&packet, '-');
	chsnprintf(temp, sizeof(temp), "%d", config->ssid);
	ax25_send_string(&packet, temp);

	// Padding
	uint8_t length = strlen(config->callsign) + (config->ssid/10);
	for(uint8_t i=length; i<7; i++)
		ax25_send_string(&packet, " ");

	ax25_send_string(&packet, ":"); // Message separator

	switch(type) {
		case CONFIG_PARM: // Telemetry parameter names

			ax25_send_string(&packet, "PARM.");

			for(uint8_t i=0; i<5; i++) {
				switch(config->tel[i]) {
					case TEL_SATS:		ax25_send_string(&packet, "Sats");			break;
					case TEL_TTFF:		ax25_send_string(&packet, "TTFF");			break;
					case TEL_VBAT:		ax25_send_string(&packet, "Vbat");			break;
					case TEL_VSOL:		ax25_send_string(&packet, "Vsol");			break;
					case TEL_PBAT:		ax25_send_string(&packet, "Pbat");			break;
					case TEL_PSOL:		ax25_send_string(&packet, "Psol");			break;
					case TEL_HUM:		ax25_send_string(&packet, "Humidity");		break;
					case TEL_PRESS:		ax25_send_string(&packet, "Airpressure");	break;
					case TEL_TEMP:		ax25_send_string(&packet, "Temperature");	break;
				}
				if(i < 4)
					ax25_send_string(&packet, ",");
			}

			break;

		case CONFIG_UNIT: // Telemetry units

			ax25_send_string(&packet, "UNIT.");

			for(uint8_t i=0; i<5; i++) {
				switch(config->tel[i]) {
					case TEL_SATS:
						break; // No unit

					case TEL_TTFF:
						ax25_send_string(&packet, "sec");
						break;

					case TEL_VBAT:
					case TEL_VSOL:
						ax25_send_string(&packet, "V");
						break;

					case TEL_PBAT:
					case TEL_PSOL:
						ax25_send_string(&packet, "W");
						break;

					case TEL_HUM:
						ax25_send_string(&packet, "%");
						break;

					case TEL_PRESS:
						ax25_send_string(&packet, "Pa");
						break;
						
					case TEL_TEMP:
						ax25_send_string(&packet, "degC");
						break;
				}
				if(i < 4)
					ax25_send_string(&packet, ",");
			}

			break;

		case CONFIG_EQNS: // Telemetry conversion parameters

			ax25_send_string(&packet, "EQNS.");

			for(uint8_t i=0; i<5; i++) {
				switch(config->tel[i]) {
					case TEL_SATS:
					case TEL_TTFF:
						ax25_send_string(&packet, "0,1,0");
						break;

					case TEL_PBAT:
					case TEL_PSOL:
					case TEL_VBAT:
					case TEL_VSOL:
						ax25_send_string(&packet, "0,.001,0");
						break;

					case TEL_HUM:
						ax25_send_string(&packet, "0,.1,0");
						break;

					case TEL_PRESS:
						ax25_send_string(&packet, "0,12.5,500");
						break;
						
					case TEL_TEMP:
						ax25_send_string(&packet, "0,.1,-100");
						break;
				}
				if(i < 4)
					ax25_send_string(&packet, ",");
			}

			break;

		case CONFIG_BITS:
			ax25_send_string(&packet, "BITS.11111111,");
			ax25_send_string(&packet, config->tel_comment);
			break;
	}

	ax25_send_footer(&packet); // Footer
	scramble(&packet);
	nrzi_encode(&packet);
	
	return packet.size;
}

