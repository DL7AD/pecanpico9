#include "config.h"
#include "debug.h"

module_conf_t config[9];
uint8_t ssdv_buffer[1024*50];
uint8_t ssdv_buffer2[1024*100];

// Put your configuration settings here
void start_user_modules(void)
{
	// Module POSITION, APRS 2m AFSK
	/*config[0].power = 20;									// Power 20 dBm
	config[0].protocol = PROT_APRS_AFSK;					// Protocol APRS, modulation AFSK
	config[0].frequency.type = FREQ_DYNAMIC;				// Dynamic frequency allocation
	config[0].frequency.hz = 144800000;						// Default frequency 144.800 MHz
	config[0].frequency.method = APRS_REGION_FREQ_2M;		// Determine local APRS frequency on 2m
	config[0].init_delay = 0;								// Module startup delay in msec
	config[0].trigger.type = TRIG_EVENT;					// Trigger transmission on event
	config[0].trigger.event = EVENT_NEW_POINT;				// Trigger when new track point released
	chsnprintf(config[0].aprs_config.callsign, 6, "DL7AD");	// APRS Callsign
	config[0].aprs_config.ssid = 12;						// APRS SSID
	config[0].aprs_config.symbol = SYM_BALLOON;				// APRS Symbol
	chsnprintf(config[0].aprs_config.path, 16, "WIDE1-1");	// APRS Path
	config[0].aprs_config.preamble = 300;					// APRS Preamble
	config[0].aprs_config.tel[0] = TEL_VBAT;				// APRS Telemetry parameter 1
	config[0].aprs_config.tel[1] = TEL_VSOL;				// APRS Telemetry parameter 2
	config[0].aprs_config.tel[2] = TEL_PRESS;				// APRS Telemetry parameter 3
	config[0].aprs_config.tel[3] = TEL_TEMP;				// APRS Telemetry parameter 4
	config[0].aprs_config.tel[4] = TEL_PBAT;				// APRS Telemetry parameter 5
	config[0].aprs_config.tel_encoding = TRUE;				// Transmit Telemetry encoding information activated
	config[0].aprs_config.tel_encoding_cycle = 3600;		// Transmit Telemetry encoding information every 3600sec
	chsnprintf(config[0].aprs_config.tel_comment, 18, "http://tkrahn.org");// Telemetry comment
	start_position_thread(&config[0]);*/

	// Module POSITION, APRS 2m 2GFSK
	/*config[1].power = 20;									// Power 10 dBm
	config[1].protocol = PROT_APRS_2GFSK;					// Protocol APRS, modulation 2GFSK
	config[1].frequency.type = FREQ_STATIC;					// Dynamic frequency allocation
	config[1].frequency.hz = 144860000;						// Default frequency 144.860 MHz
	config[1].init_delay = 0;								// Module startup delay in msec
	config[1].trigger.type = TRIG_EVENT;					// Trigger transmission on event
	config[1].trigger.event = EVENT_NEW_POINT;				// Trigger when new track point released
	chsnprintf(config[1].aprs_config.callsign, 7, "DL4MDW");// APRS Callsign
	config[1].aprs_config.ssid = 11;						// APRS SSID
	config[1].aprs_config.symbol = SYM_BALLOON;				// APRS Symbol
	chsnprintf(config[1].aprs_config.path, 16, "WIDE1-1");	// APRS Path
	config[1].aprs_config.preamble = 40;					// APRS Preamble
	config[1].aprs_config.tel[0] = TEL_VBAT;				// APRS Telemetry parameter 1
	config[1].aprs_config.tel[1] = TEL_PRESS;				// APRS Telemetry parameter 2
	config[1].aprs_config.tel[2] = TEL_TEMP;				// APRS Telemetry parameter 3
	config[1].aprs_config.tel[3] = TEL_HUM;					// APRS Telemetry parameter 4
	config[1].aprs_config.tel[4] = TEL_PBAT;				// APRS Telemetry parameter 5
	config[1].aprs_config.tel_encoding = TRUE;				// Transmit Telemetry encoding information enabled
	config[1].aprs_config.tel_encoding_cycle = 3600;		// Transmit Telemetry encoding information every 3600sec
	chsnprintf(config[1].aprs_config.tel_comment, 18, "http://tkrahn.net");// Telemetry comment
	start_position_thread(&config[1]);*/

	// Module POSITION, UKHAS 2m 2FSK
	/*config[2].power = 20;									// Power 10 dBm
	config[2].protocol = PROT_UKHAS_2FSK;					// Protocol UKHAS, modulation 2FSK
	config[2].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[2].frequency.hz = 144860000;						// Transmission frequency 144.860 MHz
	config[2].init_delay = 2000;							// Module startup delay in msec
	config[2].trigger.type = TRIG_EVENT;					// Trigger transmission on event
	config[2].trigger.event = EVENT_NEW_POINT;				// Trigger when new track point released
	config[2].fsk_config.bits = 8;							// 8bit
	config[2].fsk_config.stopbits = 2;						// 2 Stopbits
	config[2].fsk_config.predelay = 1000;					// Predelay in ms (continuos carrier before actual transmission)
	config[2].fsk_config.baud = 600;						// Baudrate
	config[2].fsk_config.shift = 1000;						// Frequency shift in Hz
	chsnprintf(config[2].ukhas_config.callsign, 6, "DK0TU");// UKHAS Callsign
	chsnprintf(config[2].ukhas_config.format, 94, "<CALL>,<ID>,<TIME>,<LAT>,<LON>,<ALT>,<SATS>,<TTFF>,<VBAT>,<PBAT>,<PRESS>,<TEMP>,<HUM>"); // UKHAS Format
	start_position_thread(&config[2]);*/

	// Module IMAGE, APRS 2m AFSK low-duty cycle
	/*config[3].power = 20;									// Power 20 dBm
	config[3].protocol = PROT_APRS_AFSK;					// Protocol APRS SSDV, modulation AFSK
	config[3].frequency.type = FREQ_DYNAMIC;				// Static frequency allocation
	config[3].frequency.hz = 144800000;						// Transmission frequency 144.800 MHz
	config[3].frequency.method = APRS_REGION_FREQ_2M;		// Determine local APRS frequency on 2m
	config[3].init_delay = 2000;							// Module startup delay in msec
	config[3].packet_spacing = 15000;						// Packet spacing in ms
	config[3].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[3].trigger.timeout = 10;							// Timeout 10 sec
	chsnprintf(config[3].aprs_config.callsign, 6, "DL7AD");	// APRS Callsign
	config[3].aprs_config.ssid = 12;						// APRS SSID
	config[3].aprs_config.preamble = 500;					// APRS Preamble
	chsnprintf(config[3].ssdv_config.callsign, 6, "DL7AD");	// SSDV Callsign
	config[3].ssdv_config.ram_buffer = ssdv_buffer;			// Camera buffer
	config[3].ssdv_config.ram_size = sizeof(ssdv_buffer);	// Buffer size
	config[3].ssdv_config.res = RES_QVGA;					// Resolution VGA
	start_image_thread(&config[3]);*/

	// Module POSITION, Morse 2m OOK
	/*config[4].power = 10;									// Power 10 dBm
	config[4].protocol = PROT_MORSE;						// Protocol Morse, modulation OOK
	config[4].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[4].frequency.hz = 144857400;						// Transmission frequency 144.8574 MHz
	config[4].init_delay = 20000;							// Module startup delay in msec
	config[4].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[4].trigger.timeout = 1200;						// Timeout 1200 sec
	config[4].ook_config.speed = 20;						// Speed 20wpm
	chsnprintf(config[4].morse_config.callsign, 6, "DK0TU");// Morse Callsign
	chsnprintf(config[4].morse_config.format, 43, "BALLOON <CALL> <LOC> <ALT>M WWW.TKRAHN.NET"); // Morse Format
	start_position_thread(&config[4]);*/

	// Module IMAGE, APRS 2m 2GFSK
	/*config[5].power = 20;									// Power 20 dBm
	config[5].protocol = PROT_APRS_2GFSK;					// Protocol APRS SSDV, modulation 2GFSK
	config[5].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[5].frequency.hz = 144860000;						// Transmission frequency 144.860 MHz
	config[5].frequency.method = FREQ_STATIC;				// Determine local APRS frequency on 2m
	config[5].init_delay = 4000;							// Module startup delay in msec
	config[5].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[5].trigger.timeout = 10;							// Timeout 10 sec
	chsnprintf(config[5].aprs_config.callsign, 6, "DL7AD");	// APRS Callsign
	config[5].aprs_config.ssid = 12;						// APRS SSID
	config[5].aprs_config.preamble = 50;					// APRS Preamble
	chsnprintf(config[5].ssdv_config.callsign, 6, "DL7AD");	// SSDV Callsign
	config[5].ssdv_config.ram_buffer = ssdv_buffer2;		// Camera buffer
	config[5].ssdv_config.ram_size = sizeof(ssdv_buffer2);	// Buffer size
	config[5].ssdv_config.res = RES_QVGA;					// Resolution XGA
	start_image_thread(&config[5]);*/

	// Module IMAGE, SSDV 2m 2FSK
	/*config[6].power = 20;									// Power 20 dBm
	config[6].protocol = PROT_SSDV_2FSK;					// Protocol SSDV, modulation 2FSK
	config[6].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[6].frequency.hz = 144860000;						// Transmission frequency 144.860 MHz
	config[6].init_delay = 30000;							// Module startup delay in msec
	config[6].packet_spacing = 6500;						// Packet spacing in ms
	config[6].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[6].trigger.timeout = 1200;						// Transmit every 900 sec
	config[6].fsk_config.bits = 8;							// 8bit
	config[6].fsk_config.stopbits = 2;						// 2 Stopbits
	config[6].fsk_config.predelay = 100;					// Predelay in ms (continuos carrier before actual transmission)
	config[6].fsk_config.baud = 600;						// Baudrate
	config[6].fsk_config.shift = 1000;						// Frequency shift in Hz
	chsnprintf(config[6].ssdv_config.callsign, 6, "DK0TU");// SSDV Callsign
	config[6].ssdv_config.ram_buffer = ssdv_buffer;		// Camera buffer
	config[6].ssdv_config.ram_size = sizeof(ssdv_buffer);	// Buffer size
	config[6].ssdv_config.res = RES_VGA;					// Resolution VGA
	start_image_thread(&config[6]);*/

	// Module LOG, APRS 2m AFSK
	/*config[8].power = 20;									// Power 20 dBm
	config[8].protocol = PROT_APRS_AFSK;					// Protocol APRS, modulation AFSK
	config[8].frequency.type = FREQ_DYNAMIC;				// Dynamic frequency allocation
	config[8].frequency.hz = 144800000;						// Default frequency 144.800 MHz
	config[8].frequency.method = APRS_REGION_FREQ_2M;		// Determine local APRS frequency on 2m
	config[8].init_delay = 10000;							// Module startup delay in msec
	config[8].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[8].trigger.timeout = 600;						// Timeout 600 sec
	chsnprintf(config[8].aprs_config.callsign, 6, "DL7AD");	// APRS Callsign
	config[8].aprs_config.ssid = 12;						// APRS SSID
	chsnprintf(config[8].aprs_config.path, 16, "WIDE1-1");	// APRS Path
	config[8].aprs_config.preamble = 500;					// APRS Preamble
	start_logging_thread(&config[8]);*/
}


