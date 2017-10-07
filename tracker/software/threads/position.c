#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "threads.h"
#include "config.h"
#include "radio.h"
#include "aprs.h"
#include "morse.h"
#include "sleep.h"
#include "chprintf.h"
#include <string.h>
#include <math.h>
#include "watchdog.h"

void str_replace(char *string, uint32_t size, char *search, char *replace) {
	for(uint32_t i=0; string[i] != 0; i++) { // Find search string
		uint32_t j=0;
		for(j=0; search[j] != 0; j++)
			if(string[i+j] != search[j])
				break;
		if(search[j] == 0) { // String found, replace it
			string[i] = 0;
			char temp[size-i-j];
			memcpy(temp, &string[i+j], size-i-j);
			chsnprintf(string, size, "%s%s%s", string, replace, temp);
			return;
		}
	}
}

uint16_t crc16(char *string) {
	size_t i;
	uint16_t crc;

	crc = 0xFFFF;

	for(i = 0; i < strlen(string); i++) {
		crc = crc ^ ((uint16_t)string[i] << 8);
		for(uint8_t j=0; j<8; j++)
		{
		    if(crc & 0x8000)
		        crc = (crc << 1) ^ 0x1021;
		    else
		        crc <<= 1;
		}
	}

	return crc;
}

void positionToMaidenhead(char m[], double lat, double lon)
{
	lon = lon + 180;
	lat = lat + 90;

	m[0] = ((uint8_t)'A') + ((uint8_t)(lon / 20));
	m[1] = ((uint8_t)'A') + ((uint8_t)(lat / 10));

	m[2] = ((uint8_t)'0') + ((uint8_t)(fmod(lon, 20)/2));
	m[3] = ((uint8_t)'0') + ((uint8_t)(fmod(lat, 10)/1));

	m[4] = ((uint8_t)'A') + ((uint8_t)((lon - ( ((uint8_t)(lon/2))*2)) / (5.0/60.0)));
	m[5] = ((uint8_t)'A') + ((uint8_t)((lat - ( ((uint8_t)(lat/1))*1)) / (2.5/60.0)));

	m[6] = 0;
}

/**
  * Replaces placeholders with variables
  */
void replace_placeholders(char* fskmsg, uint16_t size, trackPoint_t *tp) {
	char buf[16];
	chsnprintf(buf, sizeof(buf), "%d", tp->id);
	str_replace(fskmsg, size, "<ID>", buf);
	chsnprintf(buf, sizeof(buf), "%04d-%02d-%02d", tp->time.year, tp->time.month, tp->time.day);
	str_replace(fskmsg, size, "<DATE>", buf);
	chsnprintf(buf, sizeof(buf), "%02d:%02d:%02d", tp->time.hour, tp->time.minute, tp->time.second);
	str_replace(fskmsg, size, "<TIME>", buf);
	chsnprintf(buf, sizeof(buf), "%d.%05d", tp->gps_lat/10000000, ((tp->gps_lat > 0 ? 1:-1)*tp->gps_lat%10000000)/100);
	str_replace(fskmsg, size, "<LAT>", buf);
	chsnprintf(buf, sizeof(buf), "%d.%05d", tp->gps_lon/10000000, ((tp->gps_lon > 0 ? 1:-1)*tp->gps_lon%10000000)/100);
	str_replace(fskmsg, size, "<LON>", buf);
	chsnprintf(buf, sizeof(buf), "%d", tp->gps_alt);
	str_replace(fskmsg, size, "<ALT>", buf);
	chsnprintf(buf, sizeof(buf), "%d", tp->gps_sats);
	str_replace(fskmsg, size, "<SATS>", buf);
	chsnprintf(buf, sizeof(buf), "%d", tp->gps_ttff);
	str_replace(fskmsg, size, "<TTFF>", buf);
	chsnprintf(buf, sizeof(buf), "%d.%02d", tp->adc_vbat/1000, (tp->adc_vbat%1000)/10);
	str_replace(fskmsg, size, "<VBAT>", buf);
	chsnprintf(buf, sizeof(buf), "%d.%02d", tp->adc_vsol/1000, (tp->adc_vsol%1000)/10);
	str_replace(fskmsg, size, "<VSOL>", buf);
	chsnprintf(buf, sizeof(buf), "%d.%03d", tp->adc_pbat/1000, (tp->adc_pbat >= 0 ? 1 : -1) * (tp->adc_pbat%1000));
	str_replace(fskmsg, size, "<PBAT>", buf);
	chsnprintf(buf, sizeof(buf), "%d.%03d", tp->adc_rbat/1000, (tp->adc_rbat >= 0 ? 1 : -1) * (tp->adc_rbat%1000));
	str_replace(fskmsg, size, "<RBAT>", buf);
	chsnprintf(buf, sizeof(buf), "%d", tp->air_press/10);
	str_replace(fskmsg, size, "<PRESS>", buf);
	chsnprintf(buf, sizeof(buf), "%d.%d", tp->air_temp/100, (tp->air_temp%100)/10);
	str_replace(fskmsg, size, "<TEMP>", buf);
	chsnprintf(buf, sizeof(buf), "%d", tp->air_hum/10);
	str_replace(fskmsg, size, "<HUM>", buf);
	positionToMaidenhead(buf, tp->gps_lat/10000000.0, tp->gps_lon/10000000.0);
	str_replace(fskmsg, size, "<LOC>", buf);
}

THD_FUNCTION(posThread, arg) {
	module_conf_t* conf = (module_conf_t*)arg;

	trackPoint_t *trackPoint = getLastTrackPoint();
	systime_t time = chVTGetSystemTimeX();

	systime_t last_conf_transmission = chVTGetSystemTimeX();
	uint32_t current_conf_count = 0;

	while(true)
	{
		TRACE_INFO("POS  > Do module POSITION cycle");
		conf->wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		TRACE_INFO("POS  > Get last track point");
		trackPoint = getLastTrackPoint();

		if(!p_sleep(&conf->sleep_conf))
		{

			TRACE_INFO("POS  > Transmit position");

			radioMSG_t msg;
			uint8_t buffer[256];
			msg.buffer = buffer;
			msg.freq = &conf->frequency;
			msg.power = conf->power;

			switch(conf->protocol) {

				case PROT_APRS_2GFSK: // Encode APRS
				case PROT_APRS_AFSK:
					// Position transmission
					msg.mod = conf->protocol == PROT_APRS_AFSK ? MOD_AFSK : MOD_2GFSK;
					msg.gfsk_conf = &(conf->gfsk_conf);
					msg.afsk_conf = &(conf->afsk_conf);

					ax25_t ax25_handle;

					// Encode and transmit position packet
					aprs_encode_init(&ax25_handle, buffer, sizeof(buffer), msg.mod);
					aprs_encode_position(&ax25_handle, &(conf->aprs_conf), trackPoint); // Encode packet
					msg.bin_len = aprs_encode_finalize(&ax25_handle);
					transmitOnRadio(&msg, true);

					// Telemetry encoding parameter transmission
					if(conf->aprs_conf.tel_enc)
					{
						// Telemetry encoding parameter transmission trigger
						if(last_conf_transmission + S2ST(conf->aprs_conf.tel_enc_cycle) < chVTGetSystemTimeX() && current_conf_count >= 4)
						{
							last_conf_transmission += S2ST(conf->aprs_conf.tel_enc_cycle);
							current_conf_count = 0;
						}

						// Actual transmission (each cycle a different config type will be sent)
						if(conf->aprs_conf.tel_enc && current_conf_count < 4)
						{
							chThdSleepMilliseconds(5000); // Take a litte break between the package transmissions

							const telemetry_conf_t tel_conf[] = {CONF_PARM, CONF_UNIT, CONF_EQNS, CONF_BITS};

							// Encode and transmit telemetry config packet
							aprs_encode_init(&ax25_handle, buffer, sizeof(buffer), msg.mod);
							aprs_encode_telemetry_configuration(&ax25_handle, &conf->aprs_conf, tel_conf[current_conf_count]);
							msg.bin_len = aprs_encode_finalize(&ax25_handle);
							transmitOnRadio(&msg, true);

							current_conf_count++;
						}
					}

					break;

				case PROT_UKHAS_2FSK: // Encode UKHAS
					msg.mod = MOD_2FSK;
					msg.fsk_conf = &(conf->fsk_conf);

					// Encode packet
					char fskmsg[256];
					memcpy(fskmsg, conf->ukhas_conf.format, sizeof(conf->ukhas_conf.format));
					replace_placeholders(fskmsg, sizeof(fskmsg), trackPoint);
					str_replace(fskmsg, sizeof(fskmsg), "<CALL>", conf->ukhas_conf.callsign);
					msg.bin_len = 8*chsnprintf((char*)msg.buffer, sizeof(fskmsg), "$$$$$%s*%04X\n", fskmsg, crc16(fskmsg));

					// Transmit message
					transmitOnRadio(&msg, true);
					break;

				case PROT_MORSE: // Encode Morse
					msg.mod = MOD_OOK;
					msg.ook_conf = &(conf->ook_conf);

					// Encode morse message
					char morse[128];
					memcpy(morse, conf->morse_conf.format, sizeof(conf->morse_conf.format));
					replace_placeholders(morse, sizeof(morse), trackPoint);
					str_replace(morse, sizeof(morse), "<CALL>", conf->morse_conf.callsign);

					// Transmit message
					msg.bin_len = morse_encode(msg.buffer, morse); // Convert message to binary stream
					transmitOnRadio(&msg, true);
					break;

				default:
					TRACE_ERROR("POS  > Unsupported modulation/protocol selected for module POSITION");
			}
		}

		time = waitForTrigger(time, &conf->trigger);
	}
}

void start_position_thread(module_conf_t *conf)
{
	// Wait
	if(conf->init_delay) chThdSleepMilliseconds(conf->init_delay);

	// Start tracking manager (if not running yet)
	init_tracking_manager(true);

	// Start position thread
	TRACE_INFO("POS  > Startup position thread");
	chsnprintf(conf->name, sizeof(conf->name), "POS");
	thread_t *th = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(20*1024), "POS", NORMALPRIO, posThread, conf);
	if(!th) {
		// Print startup error, do not start watchdog for this thread
		TRACE_ERROR("POS  > Could not startup thread (not enough memory available)");
	} else {
		register_thread_at_wdg(conf);
		conf->wdg_timeout = chVTGetSystemTimeX() + S2ST(1);
	}
}
