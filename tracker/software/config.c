#include "config.h"
#include "aprs.h"
#include "debug.h"

module_conf_t config[9];
uint8_t ssdv_buffer[65535];
uint8_t ssdv_buffer2[65535];

/* 
 * Position module configuration description
 * =========================================
 *
 * power				int(0-127)		Defines the radio power level. It ranges from 0 (low) to 127 (high). This value is sent into the Si4464 register. The
 * (required)							real output power is operation voltage dependent. The operation voltage can be set by RUN_3V in config.h. If USB is
 *										activated the operation voltage is 3V regardsless to which value RUN_3V is set. Running the PCB at 3V following table
 *										applies:
 *										20 dBm => 127
 *										15 dBm => 40
 *										10 dBm => 20
 *										 5 dBm => 12
 *										 0 dBm => 8
 *
 * protocol				prot_t			Possible Options:
 * (required)							- PROT_APRS_AFSK	FM AFSK 1200baud transmission using the APRS protocol
 *										  this option requires aprs_conf to be set
 * 										- PROT_APRS_2GFSK	2GFSK transmission using the APRS protocol
 *										  this option requires aprs_conf and gfsk_conf to be set
 * 										- PROT_UKHAS_2FSK	Comma separated 2FSK transmission (RTTY) using the UKHAS standard
 *										  this option requires ukhas_conf and fsk_conf to be set
 * 										- PROT_MORSE		Morse transmission
 *										  this option requires morse_conf and ook_conf to be set
 *
 * frequency.type		freq_type_t		Defines the frequency type. This option will be FREQ_STATIC if not set.
 * (default FREQ_STATIC)				Possible Options:
 *										- FREQ_STATIC		Static frequency taken from frequency.hz
 *										- FREQ_APRS_REGION	Using the APRS region frequency. The tracker will change its frequency specificly to the region
 *															where it is located. e.g. 144.8MHz in Europe or 144.39MHz in the US. If the tracker doesnt know its
 *															position it takes its frequency from frequency.hz as default. Note that the tracker knows its
 *															position from its position log too. So it might use the last frequency which has been used before
 *															resetting it.
 *
 * frequency.hz			int				Frequency that this Module will transmit on (in Hz). The tracker can transmit in the 2m band. This value will be used
 * (required)							as default when frequency.type == FREQ_APRS_REGION and it doesnt know its position
 *
 * init_delay			int				Initial delay (in ms) before the module starts. This might be useful if you dont want to transmit so many APRS packets
 * (default 0ms)						at the same time on the APRS network. This option is optional. It will be 0ms if not set.
 *
 * trigger.type			trigger_type_t	Event at which this module is triggered to transmit. This option will be TRIG_ONCE if not set.
 * (default TRIG_ONCE)					Possible options:
 *										- TRIG_ONCE			Trigger once and never again (e.g. transmit specific position packet only at startup)
 *										- TRIG_NEW_POINT	Triggered when new track point available
 *										- TRIG_TIMEOUT		Triggered by timeout (e.g. trasmit position every 120sec)
 *										  this option requires trigger.timeout to be set
 *										- TRIG_CONTINOUSLY	Continue continously (e.g. send new image once old image sent completely)
 *
 * ============================== The following options are needed if protocol == PROT_APRS_AFSK or protocol == PROT_APRS_2GFSK ===============================
 *
 * aprs_conf.callsign	string			Your amateur radio callsign (this requires an amateur radio license). This callsign will be used in the APRS protocol.
 * (required)							You can transmit on the 70cm band without a license but the transmitter would need a 70cm LPF therefore.
 *
 * aprs_conf.ssid		int(0-15)		APRS SSID (no SSID = 0)
 * (default 0)
 *
 * aprs_conf.symbol		int				APRS Symbol (according to this table http://www.aprs.org/symbols.html)
 * (required)							Possible Options: SYM_BALLOON, SYM_SMALLAIRCRAFT, SYM_SATELLITE
 *
 * aprs_conf.path		string			APRS digipeating path (default: no digipeating)
 * (optional)
 *
 * aprs_conf.preamble	int				AFSK or 2GFSK preamble length (in ms). This value is required while its default is 0ms (and this would simply not work ;-) )
 * (required)
 *
 * aprs_conf.tel[0-4]	telemetry_t		There are numerous telemetry values which can be sent in the APRS position packet. One packet can contain 5 values.
 * (required)							There are possible options:
 * 										- TEL_SATS		GPS Satellites
 * 										- TEL_TTFF		Time to first fix (amount of seconds which it needed to aquire a GPS fix)
 * 										- TEL_VBAT		Battery voltage
 * 										- TEL_VSOL		Solar voltage
 * 										- TEL_PBAT		Battery power (positive charge, negative discharge)
 * 										- TEL_ISOL		Solar short current (works only if USB is unattached)
 * 										- TEL_PRESS		Air pressure (by BME280)
 * 										- TEL_TEMP		Air temperature (by BME280)
 * 										- TEL_HUM		Air humidity (by BME280)
 *
 * aprs_conf.tel_enc	bool			The telemetry in the position packets do only contain the raw values. Receivers (like aprs.fi) dont know what these
 * (default false)						values stands for. So we must tell them (e.g. that value 1 is air pressure measured in Pascal). If set to true, the
 *										tracker will transmit additional packets containing these informations. This option requires aprs_conf.tel_enc_cycle.
 *
 * aprs_conf.tel_enc_cycle	int			This values defines how often telemetry configuration packets are sent. Transmitting those packets every two
 * (default 0sec)						hours (7200sec) is a good value. This value is set in seconds.
 *
 * aprs_conf.tel_comment	string		There can be set a short comment which is sent with the APRS configuration packets. It has no affect if
 * (optional)							aprs_conf.tel_enc is set false. Its default is empty.
 *
 * ============================================= The following options are needed if protocol == PROT_APRS_2GFSK ==============================================
 *
 * gfsk_conf.speed		int				2GFSK speed. Following values have been tested successfully: 9600, 19200, 38400, 76800, 96000.
 * (required)
 *
 * ============================================= The following options are needed if protocol == PROT_UKHAS_2FSK ==============================================
 *
 * fsk_conf.bits		int(7-8)		Bits
 * (required)
 *
 * fsk_conf.stopbits	int(1-2)		Stopbits
 * (required)
 *
 * fsk_conf.predelay	int				Predelay (in ms). The receiver needs to settle on the frequency for a while. Therefore is switched on some seconds
 * (default 0ms)						before. By default its 0ms but that wouldnt work.
 *
 * fsk_conf.baud		int				Baudrate. Following values have been tested successfully: 50, 300, 600.
 * (required)
 *
 * fsk_conf.shift		int				Frequency shift of 2FSK
 * (required)
 *
 * ukhas_conf.callsign	string			Your amateur radio callsign (this requires an amateur radio license). This callsign will be used in the UKHAS protocol.
 * (required)							You can transmit on the 70cm band without a license but the transmitter would need a 70cm LPF therefore.
 *
 * ukhas_conf.format	string			UKHAS format. This is the string which will be sent. This string contains variables (e.g. <VBAT>) which will be
 * (required)							replaced by the actual values.
 *										For example: "<CALL>,<ID>,<TIME>,<LAT>,<LON>,<ALT>" will become "MYCALL,324,14:23:35,52.4537,13.9362,8462"
 *										The prefix "$$$$$" and CRC will be added automatically. Following variables can be used:
 *										- <ID>		Incremental ID
 *										- <DATE>	Date
 *										- <TIME>	Time
 *										- <LAT>		Latitude
 *										- <LON>		Longitude
 *										- <ALT>		Altitude (in meter!)
 *										- <SATS>	GPS Satellites
 *										- <TTFF>	Time to first fix
 *										- <VBAT>	Battery voltage
 *										- <VSOL>	Solar voltage
 *										- <PBAT>	Battery power consumption
 *										- <ISOL>	Solar short current
 *										- <PRESS>	Air pressure
 *										- <TEMP>	Air temperature
 *										- <HUM>		Air humidity
 *										- <LOC>		Maidenhead locator
 *
 * ================================================ The following options are needed if protocol == PROT_MORSE ================================================
 *
 * ook_conf.speed		int				Morse Speed in Wpm
 * (required)
 *
 * morse_conf.callsign	string			Your amateur radio callsign (this requires an amateur radio license). This callsign will be used in the morse "protocol".
 * (required)							You can transmit on the 70cm band without a license but the transmitter would need a 70cm LPF therefore.
 *
 * morse_conf.format	string			Morse format. This is the string which will be sent. This string contains variables (e.g. <VBAT>) which will be
 * (required)							replaced by the actual values.
 *										For example: "BALLOON <CALL> <LOC> <ALT>M" will become "BALLOON MYCALL JO62CF 9362M"
 *										The following variables can be used:
 *										- <ID>		Incremental ID
 *										- <DATE>	Date
 *										- <TIME>	Time
 *										- <LAT>		Latitude
 *										- <LON>		Longitude
 *										- <ALT>		Altitude (in meter!)
 *										- <SATS>	GPS Satellites
 *										- <TTFF>	Time to first fix
 *										- <VBAT>	Battery voltage
 *										- <VSOL>	Solar voltage
 *										- <PBAT>	Battery power consumption
 *										- <ISOL>	Solar short current
 *										- <PRESS>	Air pressure
 *										- <TEMP>	Air temperature
 *										- <HUM>		Air humidity
 *										- <LOC>		Maidenhead locator
 */

// Put your configuration settings here
void start_user_modules(void)
{
	// Module POSITION, APRS 2m AFSK
	config[0].power = 127;									// Power 20 dBm
	config[0].protocol = PROT_APRS_AFSK;					// Protocol APRS, modulation AFSK
	config[0].frequency.type = FREQ_APRS_REGION;			// Dynamic frequency allocation
	config[0].frequency.hz = 144800000;						// Default frequency 144.800 MHz
	config[0].init_delay = 0;								// Module startup delay in msec
	config[0].trigger.type = TRIG_NEW_POINT;				// Trigger when new track point released
	chsnprintf(config[0].aprs_conf.callsign, 6, "DL7AD");	// APRS Callsign
	config[0].aprs_conf.ssid = 12;							// APRS SSID
	config[0].aprs_conf.symbol = SYM_BALLOON;				// APRS Symbol
	chsnprintf(config[0].aprs_conf.path, 16, "WIDE1-1");	// APRS Path
	config[0].aprs_conf.preamble = 300;						// APRS Preamble
	config[0].aprs_conf.tel[0] = TEL_VBAT;					// APRS Telemetry parameter 1
	config[0].aprs_conf.tel[1] = TEL_TEMP;					// APRS Telemetry parameter 2
	config[0].aprs_conf.tel[2] = TEL_PRESS;					// APRS Telemetry parameter 3
	config[0].aprs_conf.tel[3] = TEL_HUM;					// APRS Telemetry parameter 4
	config[0].aprs_conf.tel[4] = TEL_SATS;					// APRS Telemetry parameter 5
	config[0].aprs_conf.tel_enc = TRUE;						// Transmit Telemetry encoding information activated
	config[0].aprs_conf.tel_enc_cycle = 3600;				// Transmit Telemetry encoding information every 3600sec
	chsnprintf(config[0].aprs_conf.tel_comment, 30, "http://ssdv.habhub.org/DL7AD");// Telemetry comment
	start_position_thread(&config[0]);

	// Module POSITION, APRS 2m AFSK
	/*config[0].power = 127;									// Power 20 dBm
	config[0].protocol = PROT_APRS_AFSK;					// Protocol APRS, modulation AFSK
	config[0].frequency.type = FREQ_APRS_REGION;			// Dynamic frequency allocation
	config[0].frequency.hz = 144800000;						// Default frequency 144.800 MHz
	config[0].init_delay = 30000;								// Module startup delay in msec
	config[0].trigger.type = TRIG_NEW_POINT;				// Trigger when new track point released
	chsnprintf(config[0].aprs_conf.callsign, 7, "DL4MDW");	// APRS Callsign
	config[0].aprs_conf.ssid = 14;							// APRS SSID
	config[0].aprs_conf.symbol = SYM_BALLOON;				// APRS Symbol
	chsnprintf(config[0].aprs_conf.path, 16, "WIDE1-1");	// APRS Path
	config[0].aprs_conf.preamble = 300;						// APRS Preamble
	config[0].aprs_conf.tel[0] = TEL_SATS;					// APRS Telemetry parameter 1
	config[0].aprs_conf.tel[1] = TEL_TTFF;					// APRS Telemetry parameter 2
	config[0].aprs_conf.tel[2] = TEL_PRESS;					// APRS Telemetry parameter 3
	config[0].aprs_conf.tel[3] = TEL_TEMP;					// APRS Telemetry parameter 4
	config[0].aprs_conf.tel[4] = TEL_HUM;					// APRS Telemetry parameter 5
	config[0].aprs_conf.tel_enc = TRUE;				// Transmit Telemetry encoding information activated
	config[0].aprs_conf.tel_enc_cycle = 3600;			// Transmit Telemetry encoding information every 3600sec
	chsnprintf(config[0].aprs_conf.tel_comment, 30, "http://ssdv.habhub.org/DL4MDW");// Telemetry comment
	start_position_thread(&config[0]);*/

	// Module POSITION, APRS 2m 2GFSK
	/*config[1].power = 127;								// Power 10 dBm
	config[1].protocol = PROT_APRS_2GFSK;					// Protocol APRS, modulation 2GFSK
	config[1].gfsk_conf.speed = 9600;						// 2GFSK Speed
	config[1].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[1].frequency.hz = 144860000;						// Default frequency 144.860 MHz
	config[1].init_delay = 0;								// Module startup delay in msec
	config[1].trigger.type = TRIG_NEW_POINT;				// Trigger when new track point released
	chsnprintf(config[1].aprs_conf.callsign, 7, "DL4MDW");	// APRS Callsign
	config[1].aprs_conf.ssid = 11;							// APRS SSID
	config[1].aprs_conf.symbol = SYM_BALLOON;				// APRS Symbol
	chsnprintf(config[1].aprs_conf.path, 16, "WIDE1-1");	// APRS Path
	config[1].aprs_conf.preamble = 40;						// APRS Preamble
	config[0].aprs_conf.tel[0] = TEL_VBAT;					// APRS Telemetry parameter 1
	config[0].aprs_conf.tel[1] = TEL_VSOL;					// APRS Telemetry parameter 2
	config[0].aprs_conf.tel[2] = TEL_PRESS;					// APRS Telemetry parameter 3
	config[0].aprs_conf.tel[3] = TEL_TEMP;					// APRS Telemetry parameter 4
	config[0].aprs_conf.tel[4] = TEL_PBAT;					// APRS Telemetry parameter 5
	config[1].aprs_conf.tel_enc = TRUE;				// Transmit Telemetry encoding information enabled
	config[1].aprs_conf.tel_enc_cycle = 3600;			// Transmit Telemetry encoding information every 3600sec
	chsnprintf(config[1].aprs_conf.tel_comment, 18, "http://tkrahn.net");// Telemetry comment
	start_position_thread(&config[1]);*/

	// Module POSITION, UKHAS 2m 2FSK
	/*config[2].power = 127;								// Power 10 dBm
	config[2].protocol = PROT_UKHAS_2FSK;					// Protocol UKHAS, modulation 2FSK
	config[2].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[2].frequency.hz = 144860000;						// Transmission frequency 144.860 MHz
	config[2].init_delay = 2000;							// Module startup delay in msec
	config[2].trigger.type = TRIG_NEW_POINT;				// Trigger when new track point released
	config[2].fsk_conf.bits = 8;							// 8bit
	config[2].fsk_conf.stopbits = 2;						// 2 Stopbits
	config[2].fsk_conf.predelay = 1000;						// Predelay in ms (continuos carrier before actual transmission)
	config[2].fsk_conf.baud = 600;							// Baudrate
	config[2].fsk_conf.shift = 1000;						// Frequency shift in Hz
	chsnprintf(config[2].ukhas_conf.callsign, 6, "DK0TU");	// UKHAS Callsign
	chsnprintf(config[2].ukhas_conf.format, 94, "<CALL>,<ID>,<TIME>,<LAT>,<LON>,<ALT>,<SATS>,<TTFF>,<VBAT>,<PBAT>,<PRESS>,<TEMP>,<HUM>"); // UKHAS Format
	start_position_thread(&config[2]);*/

	// Module IMAGE, APRS 2m AFSK low-duty cycle
	config[3].power = 127;									// Power 20 dBm
	config[3].protocol = PROT_APRS_AFSK;					// Protocol APRS SSDV, modulation AFSK
	config[3].frequency.type = FREQ_APRS_REGION;			// Dynamic frequency allocation
	config[3].frequency.hz = 144800000;						// Transmission frequency 144.800 MHz
	config[3].init_delay = 10000;							// Module startup delay in msec
	config[3].packet_spacing = 20000;						// Packet spacing in ms
	config[3].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[3].trigger.timeout = 10;							// Timeout 10 sec
	chsnprintf(config[3].aprs_conf.callsign, 7, "DL7AD");	// APRS Callsign
	config[3].aprs_conf.ssid = 12;							// APRS SSID
	config[3].aprs_conf.preamble = 300;						// APRS Preamble
	chsnprintf(config[3].ssdv_conf.callsign, 7, "DL7AD");	// SSDV Callsign
	config[3].ssdv_conf.ram_buffer = ssdv_buffer;			// Camera buffer
	config[3].ssdv_conf.ram_size = sizeof(ssdv_buffer);		// Buffer size
	config[3].ssdv_conf.res = RES_QVGA;						// Resolution VGA
	config[3].ssdv_conf.redundantTx = true;					// Transmit packets twice
	start_image_thread(&config[3]);

	// Module POSITION, Morse 2m OOK
	/*config[4].power = 127;								// Power 10 dBm
	config[4].protocol = PROT_MORSE;						// Protocol Morse, modulation OOK
	config[4].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[4].frequency.hz = 144857400;						// Transmission frequency 144.8574 MHz
	config[4].init_delay = 20000;							// Module startup delay in msec
	config[4].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[4].trigger.timeout = 1200;						// Timeout 1200 sec
	config[4].ook_conf.speed = 20;							// Speed 20wpm
	chsnprintf(config[4].morse_conf.callsign, 6, "DK0TU");	// Morse Callsign
	chsnprintf(config[4].morse_conf.format, 43, "BALLOON <CALL> <LOC> <ALT>M WWW.TKRAHN.NET"); // Morse Format
	start_position_thread(&config[4]);*/

	// Module IMAGE, APRS 2m 2GFSK
	/*config[5].power = 127;									// Power 20 dBm
	config[5].protocol = PROT_APRS_2GFSK;					// Protocol APRS SSDV, modulation 2GFSK
	config[5].gfsk_conf.speed = 9600;						// 2GFSK Speed
	config[5].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[5].frequency.hz = 144860000;						// Transmission frequency 144.860 MHz
	config[5].init_delay = 120000;							// Module startup delay in msec
	config[5].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[5].trigger.timeout = 2;							// Timeout 10 sec
	chsnprintf(config[5].aprs_conf.callsign, 6, "DL7AD");	// APRS Callsign
	config[5].aprs_conf.ssid = 12;							// APRS SSID
	config[5].aprs_conf.preamble = 50;						// APRS Preamble
	chsnprintf(config[5].ssdv_conf.callsign, 6, "DL7AD");	// SSDV Callsign
	config[5].ssdv_conf.ram_buffer = ssdv_buffer2;			// Camera buffer
	config[5].ssdv_conf.ram_size = sizeof(ssdv_buffer2);	// Buffer size
	config[5].ssdv_conf.res = RES_QVGA;						// Resolution XGA
	config[5].ssdv_conf.redundantTx = true;					// Transmit packets twice
	start_image_thread(&config[5]);*/

	// Module IMAGE, SSDV 2m 2FSK
	/*config[6].power = 127;								// Power 20 dBm
	config[6].protocol = PROT_SSDV_2FSK;					// Protocol SSDV, modulation 2FSK
	config[6].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[6].frequency.hz = 144860000;						// Transmission frequency 144.860 MHz
	config[6].init_delay = 30000;							// Module startup delay in msec
	config[6].packet_spacing = 6500;						// Packet spacing in ms
	config[6].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[6].trigger.timeout = 1200;						// Transmit every 900 sec
	config[6].fsk_conf.bits = 8;							// 8bit
	config[6].fsk_conf.stopbits = 2;						// 2 Stopbits
	config[6].fsk_conf.predelay = 100;						// Predelay in ms (continuos carrier before actual transmission)
	config[6].fsk_conf.baud = 600;							// Baudrate
	config[6].fsk_conf.shift = 1000;						// Frequency shift in Hz
	chsnprintf(config[6].ssdv_conf.callsign, 6, "DK0TU");	// SSDV Callsign
	config[6].ssdv_conf.ram_buffer = ssdv_buffer;			// Camera buffer
	config[6].ssdv_conf.ram_size = sizeof(ssdv_buffer);		// Buffer size
	config[6].ssdv_conf.res = RES_VGA;						// Resolution VGA
	start_image_thread(&config[6]);*/

	// Module LOG, APRS 2m AFSK
	/*config[8].power = 127;									// Power 20 dBm
	config[8].protocol = PROT_APRS_AFSK;					// Protocol APRS, modulation AFSK
	config[8].frequency.type = FREQ_APRS_REGION;			// Dynamic frequency allocation
	config[8].frequency.hz = 144800000;						// Default frequency 144.800 MHz
	config[8].init_delay = 10000;							// Module startup delay in msec
	config[8].trigger.type = TRIG_TIMEOUT;					// Trigger transmission on timeout (Periodic cycling)
	config[8].trigger.timeout = 180;						// Timeout 600 sec
	chsnprintf(config[8].aprs_conf.callsign, 7, "DL4MDW");	// APRS Callsign
	config[8].aprs_conf.ssid = 12;							// APRS SSID
	chsnprintf(config[8].aprs_conf.path, 16, "WIDE1-1");	// APRS Path
	config[8].aprs_conf.preamble = 300;						// APRS Preamble
	start_logging_thread(&config[8]);*/
}


