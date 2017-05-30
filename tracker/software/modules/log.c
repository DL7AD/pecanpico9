/**
  * Logging module
  * 
  */

#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "modules.h"
#include "base.h"
#include "aprs.h"
#include <math.h>

static uint16_t pkt[LOG_TRANSMISSION_TIME*3+2];
static uint8_t pkt_base91[BASE91LEN(sizeof(pkt))];
static uint32_t nextLogEntryCounter; // Current log pointer (determines next log transmission)

/**
  * Lookup table of NL for the CPR algorithm
  * See ICAO DO-260B for more details
  */
uint8_t lookupNL(double lat)
{
	// FIXME: Change this mess to a real lookup table
	double a = fabs(lat);
	if     (a < 10.47047130) return 59;
	else if(a < 14.82817437) return 58;
	else if(a < 18.18626357) return 57;
	else if(a < 21.02939493) return 56;
	else if(a < 23.54504487) return 55;
	else if(a < 25.82924707) return 54;
	else if(a < 27.93898710) return 53;
	else if(a < 29.91135686) return 52;
	else if(a < 31.77209708) return 51;
	else if(a < 33.53993436) return 50;
	else if(a < 35.22899598) return 49;
	else if(a < 36.85025108) return 48;
	else if(a < 38.41241892) return 47;
	else if(a < 39.92256684) return 46;
	else if(a < 41.38651832) return 45;
	else if(a < 42.80914012) return 44;
	else if(a < 44.19454951) return 43;
	else if(a < 45.54626723) return 42;
	else if(a < 46.86733252) return 41;
	else if(a < 48.16039128) return 40;
	else if(a < 49.42776439) return 39;
	else if(a < 50.67150166) return 38;
	else if(a < 51.89342469) return 37;
	else if(a < 53.09516153) return 36;
	else if(a < 54.27817472) return 35;
	else if(a < 55.44378444) return 34;
	else if(a < 56.59318756) return 33;
	else if(a < 57.72747354) return 31;
	else if(a < 58.84763776) return 30;
	else if(a < 59.95459277) return 30;
	else if(a < 61.04917774) return 29;
	else if(a < 62.13216659) return 28;
	else if(a < 63.20427479) return 27;
	else if(a < 64.26616523) return 26;
	else if(a < 65.31845310) return 25;
	else if(a < 66.36171008) return 24;
	else if(a < 67.39646774) return 23;
	else if(a < 68.42322022) return 22;
	else if(a < 69.44242631) return 21;
	else if(a < 70.45451075) return 20;
	else if(a < 71.45986473) return 19;
	else if(a < 72.45884545) return 18;
	else if(a < 73.45177442) return 17;
	else if(a < 74.43893416) return 16;
	else if(a < 75.42056257) return 15;
	else if(a < 76.39684391) return 14;
	else if(a < 77.36789461) return 13;
	else if(a < 78.33374083) return 12;
	else if(a < 79.29428225) return 11;
	else if(a < 80.24923213) return 10;
	else if(a < 81.19801349) return 9;
	else if(a < 82.13956981) return 8;
	else if(a < 83.07199445) return 7;
	else if(a < 83.99173563) return 6;
	else if(a < 84.89166191) return 5;
	else if(a < 85.75541621) return 4;
	else if(a < 86.53536998) return 3;
	else if(a < 87.00000000) return 2;
	else return 1;
}

/**
  * Compressed position reporting format encoding
  * CPR Parameters nz=60, nb=8, na=360
  * See ICAO DO-260B for more details
  */
uint16_t cpr_encode(bool cprFormat, double lat, double lon)
{
	// Convert coordinates
	if(lat < 0)
		lat += 360; // Southern hemisphere is shown as 270-360 degree

	double dlat[] = {6.0, 360.0/59.0};
	uint8_t yz = floor(256 * fmod(lat, dlat[cprFormat]) / dlat[cprFormat] + 0.5);
	double rlat = dlat[cprFormat] * (yz / 256 + floor(lat / dlat[cprFormat]));
	uint8_t nl = lookupNL(rlat);
	double dlon = nl-cprFormat > 0 ? 360.0 / (nl-cprFormat) : 360.0;
	uint8_t xz = floor(256 * fmod(lon, dlon) / dlon + 0.5);
	
	return (yz << 8) | xz;
}

THD_FUNCTION(moduleLOG, arg)
{
	module_conf_t* config = (module_conf_t*)arg;

	// Execute Initial delay
	if(config->init_delay)
		chThdSleepMilliseconds(config->init_delay);

	// Print initialization message
	TRACE_INFO("LOG  > Startup module %s", config->name);

	systime_t time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("LOG  > Do module LOG cycle");
		config->last_update = chVTGetSystemTimeX(); // Update Watchdog timer

		if(!p_sleep(&config->sleep_config))
		{
			// Get log from memory
			logTrackPoint_t log;
			getLogTrackPoints(&log, nextLogEntryCounter++, 1);

			// Encode absolute time
			pkt[0] = log.time >> 16;
			pkt[1] = log.time & 0xFFFF;
			uint32_t rel_time = log.time;

			// Encode log points
			for(uint32_t i=0,z=0; i<LOG_TRANSMISSION_TIME; i++,z++)
			{
				getLogTrackPoints(&log, nextLogEntryCounter++, 1); // Read log point

				if(z == LOG_FLASH_SIZE / sizeof(logTrackPoint_t)) // No entry in log memory
					break;

				if(log.time == 0xFFFFFFFF) { // Log entry invalid (not set)
					i--;
					continue;
				}

				TRACE_INFO("LOG  > Encode log ID=%d", (nextLogEntryCounter-1) % (LOG_FLASH_SIZE / sizeof(logTrackPoint_t)));

				pkt[i*3+2] = log.time - rel_time; // Time difference to last log point
				pkt[i*3+3] = cpr_encode(i%2, log.gps_lat/10000000.0, log.gps_lon/10000000.0);
				pkt[i*3+4] = log.gps_alt; // Altitude in meters				

				rel_time = log.time; // Set absolute time for next log point
			}

			// Encode radio message
			radioMSG_t msg;
			msg.freq = getFrequency(&config->frequency);
			msg.power = config->power;

			switch(config->protocol) {
				case PROT_APRS_2GFSK:
				case PROT_APRS_AFSK:
					msg.mod = config->protocol == PROT_APRS_AFSK ? MOD_AFSK : MOD_2GFSK;
					msg.afsk_config = &(config->afsk_config);
					msg.gfsk_config = &(config->gfsk_config);

					// Deleting buffer
					for(uint16_t t=0; t<sizeof(pkt_base91); t++)
						pkt_base91[t] = 0;

					base91_encode((uint8_t*)pkt, pkt_base91, sizeof(pkt));
					msg.bin_len = aprs_encode_message(msg.msg, msg.mod, &config->aprs_config, APRS_DEST_CALLSIGN, (char*)pkt_base91);

					transmitOnRadio(&msg);
					break;

				default:
					TRACE_ERROR("LOG  > Unsupported protocol selected for module LOG");
			}
		}

		time = waitForTrigger(time, &config->trigger);
	}
}

