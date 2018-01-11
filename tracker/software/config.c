/* 
 * Position module configuration description
 * =========================================
 *
 * This module activates the position transmission of the tracker. If one of the position modules activated, it will activate the tracking manager which
 * samples GPS position and atmospheric data. If you want to save position data to the flash memory, you have to enable at lease one position module. You can
 * transmit positions either using 2FSK (RTTY) with the UKHAS standard, CW (Morse) or APRS (AFSK or 2GFSK).
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
 *										- TRIG_CONTINUOUSLY	Continue continuously (e.g. send new image once old image sent completely)
 *
 * trigger.timeout		int				Amount of seconds of module cycle (in seconds). This option is only neccessary if trigger.type == TRIG_TIMEOUT.
 * (default 0s)
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
 * 										- TEL_VBAT		Battery voltage (in mV)
 * 										- TEL_VSOL		Solar voltage (in mV)
 * 										- TEL_PBAT		Battery power (positive charge, negative discharge)
 * 										- TEL_RBAT		Battery impedance (in mOhm)
 * 										- TEL_PRESS		Air pressure (in Pa)
 * 										- TEL_TEMP		Air temperature (in degC*100)
 * 										- TEL_HUM		Air humidity (in %*10)
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
 * gfsk_conf.speed		int				2GFSK speed. Following values have been tested successfully: 9600, 19200.
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


/*
 * Image module configuration description
 * ======================================
 *
 * This module activates the transmission of pictures from the camera. You can use either SSDV transsmitted with 2FSK (RTTY) or APRS/SSDV. If you choose
 * APRS/SSDV, generic APRS packets will be transmitted which can be received by any APRS receiver and decoded with a computer. In order to decode 2FSK/SSDV
 * please use DL-FlDigi (https://ukhas.org.uk/projects:dl-fldigi). 2FSK/SSDV is very stable even with low receiption signals but it needs a SSB receiver. If
 * you want to use APRS/SSDV you are going to need a packet receiver (like TH-D72, TH-D74 or FT2D or generic TNC). The packets can be decoded by a python
 * script in the /decoder folder of this repository.
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
 * (required)							- PROT_APRS_AFSK	FM AFSK 1200baud transmission using the APRS/SSDV protocol
 *										  this option requires aprs_conf to be set
 * 										- PROT_APRS_2GFSK	2GFSK transmission using the APRS/SSDV protocol
 *										  this option requires aprs_conf and gfsk_conf to be set
 * 										- PROT_SSDV_2FSK	2FSK transmission using the SSDV protocol
 *										  this option requires fsk_conf to be set
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
 *										- TRIG_CONTINUOUSLY	Continue continuously (e.g. send new image once old image sent completely)
 *
 * trigger.timeout		int				Amount of seconds of module cycle (in seconds). This option is only neccessary if trigger.type == TRIG_TIMEOUT.
 * (default 0s)
 *
 * ssdv_conf.callsign	string			The SSDV callsign (or stream identifier). This value helps the SSDV algorithm to assign packets from different images
 * (required)							to the right data set. This is helpful if multiple modules transmit different images at the same time.
 *
 * ssdv_conf.ram_buffer	data			Array of bytes which is used by the module for buffering the image
 * (required)
 *
 * ssdv_conf.ram_size					Size of buffer => sizeof(ssdv_conf.ram_buffer)
 * (required)
 *
 * ssdv_conf.res		resolution_t	Resolution of the image
 * (default RES_QVGA)					Possible options:
 *										- RES_QVGA	QVGA Resolution (320x240px)
 *										- RES_VGA	VGA Resolution (640x480px)
 *										- RES_XGA	XGA Resolution (1204x768px)
 *										- RES_UXGA	UXGA Resolution (1600x1200px)
 *										- RES_MAX	The module samples the highest resolution which fits into ssdv_conf.ram_buffer.
 *
 * ssdv_conf.redundantTx	bool		Enables redudant packet transmission if set to true. This option will enable the packets to be transmitted twice.
 *
 * ssdv_conf.quality	int(0-7)		Quality (quantization) of the JPEG algorithm. It can be set from 0 (low quality) to 7 (high quality). (Recommended: 4)
 *
 * ============================== The following options are needed if protocol == PROT_APRS_AFSK or protocol == PROT_APRS_2GFSK ===============================
 *
 * aprs_conf.callsign	string			Your amateur radio callsign (this requires an amateur radio license). This callsign will be used in the APRS protocol.
 * (required)							You can transmit on the 70cm band without a license but the transmitter would need a 70cm LPF therefore.
 *
 * aprs_conf.ssid		int(0-15)		APRS SSID (no SSID = 0)
 * (default 0)
 *
 * aprs_conf.path		string			APRS digipeating path (default: no digipeating)
 * (optional)
 *
 * aprs_conf.preamble	int				AFSK or 2GFSK preamble length (in ms). This value is required while its default is 0ms (and this would simply not work ;-) )
 * (required)
 *
 * ============================================= The following options are needed if protocol == PROT_APRS_2GFSK ==============================================
 *
 * gfsk_conf.speed		int				2GFSK speed. Following values have been tested successfully: 9600, 19200.
 * (required)
 *
 * ============================================== The following options are needed if protocol == PROT_SSDV_2FSK ==============================================
 *
 * fsk_conf.bits		int(7-8)		Bits
 * (required)
 *
 * fsk_conf.stopbits	int(1-2)		Stopbits (Important: DL-FlDigi can decode SSDV only using 2 stopbits!)
 * (required)
 *
 * fsk_conf.predelay	int				Predelay (in ms). The receiver needs to settle on the frequency for a while. Therefore is switched on some seconds
 * (default 0ms)						before. By default its 0ms but that wouldnt work.
 *
 * fsk_conf.baud		int				Baudrate. Following values have been tested successfully: 50, 300, 600. (Recommended: 300 or 600)
 * (required)
 *
 * fsk_conf.shift		int				Frequency shift of 2FSK
 * (required)
 */


/* 
 * Log module configuration description
 * ====================================
 *
 * This module activates log (track point) transmission over APRS (AFSK or 2GFSK). This module can be used to receive log points while the tracker has been out
 * of range from the APRS network.
 * Note: Track points are saved on the flash memory without activation of the Log module. They are just not sent out.
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
 *										- TRIG_CONTINUOUSLY	Continue continuously (e.g. send new image once old image sent completely)
 *
 * trigger.timeout		int				Amount of seconds of module cycle (in seconds). This option is only neccessary if trigger.type == TRIG_TIMEOUT.
 * (default 0s)
 *
 * aprs_conf.callsign	string			Your amateur radio callsign (this requires an amateur radio license). This callsign will be used in the APRS protocol.
 * (required)							You can transmit on the 70cm band without a license but the transmitter would need a 70cm LPF therefore.
 *
 * aprs_conf.ssid		int(0-15)		APRS SSID (no SSID = 0)
 * (default 0)
 *
 * aprs_conf.path		string			APRS digipeating path (default: no digipeating)
 * (optional)
 *
 * aprs_conf.preamble	int				AFSK or 2GFSK preamble length (in ms). This value is required while its default is 0ms (and this would simply not work ;-) )
 * (required)
 *
 * ============================================= The following options are needed if protocol == PROT_APRS_2GFSK ==============================================
 *
 * gfsk_conf.speed		int				2GFSK speed. Following values have been tested successfully: 9600, 19200.
 * (required)
 */


// Put your configuration settings here

// Global variables
#include "config.h"
#include "aprs.h"
#include "image.h"
#include "position.h"
#include "log.h"
#include "chprintf.h"

module_conf_t config[7];

uint8_t ssdv_buffer[256*1024] __attribute__((aligned(32)));	// Image buffer

systime_t track_cycle_time = S2ST(60);						// Tracking cycle (all peripheral data [airpressure, GPS, temperature, ...] is collected each 60 seconds
bool keep_cam_switched_on =	false;							// Keep camera switched on and initialized, this makes image capturing faster but takes a lot of power over long time
uint16_t gps_on_vbat = 3000;								// Battery voltage threshold at which GPS is switched on
uint16_t gps_off_vbat = 2500;								// Battery voltage threshold at which GPS is switched off
uint16_t gps_onper_vbat = 3000;								// Battery voltage threshold at which GPS is kept switched on all time. This value must be larger
															// than gps_on_vbat and gps_off_vbat otherwise this value has no effect. Value 0 disables this feature

void start_user_modules(void)
{
	/*
	 * Use one of these example blocks by uncommenting the start_*_thread() line.
	 */

	/* -------------------------------------------------- POSITION TRANSMISSION -------------------------------------------------- */

	// Module POSITION, APRS 2m AFSK
	config[0].power = 127;									// Transmission Power
	config[0].protocol = PROT_APRS_AFSK;					// Protocol APRS (AFSK)
	config[0].frequency.type = FREQ_APRS_REGION;			// Dynamic frequency allocation
	config[0].frequency.hz = 144800000;						// Default frequency 144.800 MHz
	config[0].trigger.type = TRIG_NEW_POINT;				// Transmit when tracking manager samples new tracking point
	chsnprintf(config[0].aprs_conf.callsign, 16, "DL7AD");	// APRS Callsign
	config[0].aprs_conf.ssid = 15;							// APRS SSID
	config[0].aprs_conf.symbol = SYM_BALLOON;				// APRS Symbol
	chsnprintf(config[0].aprs_conf.path, 16, "WIDE1-1");	// APRS Path
	config[0].aprs_conf.preamble = 200;						// APRS Preamble (200ms)
	config[0].aprs_conf.tel[0] = TEL_VBAT;					// APRS Telemetry parameter 1: Battery voltage
	config[0].aprs_conf.tel[1] = TEL_VSOL;					// APRS Telemetry parameter 2: Solar voltage
	config[0].aprs_conf.tel[2] = TEL_PBAT;					// APRS Telemetry parameter 3: Battery charge/discharge power
	config[0].aprs_conf.tel[3] = TEL_TEMP;					// APRS Telemetry parameter 4: Temperature
	config[0].aprs_conf.tel[4] = TEL_PRESS;					// APRS Telemetry parameter 5: Airpressuse
	config[0].aprs_conf.tel_enc = TRUE;						// Transmit Telemetry encoding information activated
	config[0].aprs_conf.tel_enc_cycle = 3600;				// Transmit Telemetry encoding information every 3600sec
	chsnprintf(config[0].aprs_conf.tel_comment, 30, "http://ssdv.habhub.org/DL7AD");// Telemetry comment
	start_position_thread(&config[0]);

	// Module POSITION, UKHAS 2m 2FSK
	config[1].power = 127;									// Transmission Power
	config[1].protocol = PROT_UKHAS_2FSK;					// Protocol UKHAS (2FSK)
	config[1].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[1].frequency.hz = 144160000;						// Transmission frequency 144.160 MHz
	config[1].trigger.type = TRIG_CONTINUOUSLY;				// Transmit continuously
	config[1].fsk_conf.bits = 8;							// 8 bit
	config[1].fsk_conf.stopbits = 2;						// 2 stopbits
	config[1].fsk_conf.predelay = 3000;						// Preamble (1000ms)
	config[1].fsk_conf.baud = 50;							// Baudrate
	config[1].fsk_conf.shift = 425;							// Frequency shift in Hz
	chsnprintf(config[1].ukhas_conf.callsign, 16, "DL7AD");	// UKHAS Callsign
	chsnprintf(config[1].ukhas_conf.format, 150, "<CALL>,<ID>,<TIME>,<LAT>,<LON>,<ALT>,<SATS>,<TTFF>,<VBAT>,<PBAT>,<PRESS>,<TEMP>,<HUM>"); // UKHAS Format
	//start_position_thread(&config[1]);

	// Module POSITION, Morse 2m OOK
	config[2].power = 127;									// Transmission Power
	config[2].protocol = PROT_MORSE;						// Protocol Morse (OOK)
	config[2].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[2].frequency.hz = 144160000;						// Transmission frequency 144.160 MHz
	config[2].trigger.type = TRIG_TIMEOUT;					// Periodic cycling (every 60 seconds)
	config[2].trigger.timeout = 60;							// Timeout 60 sec
	config[2].ook_conf.speed = 20;							// Speed 20 wpm
	chsnprintf(config[2].morse_conf.callsign, 16, "DL7AD");	// Morse Callsign
	chsnprintf(config[2].morse_conf.format, 50, "BALLOON <CALL> <LOC> <ALT>M"); // Morse Format
	//start_position_thread(&config[2]);



	/* ---------------------------------------------------- IMAGE TRANSMISSION --------------------------------------------------- */

	// Module IMAGE, APRS 2m AFSK low-duty cycle
	config[3].power = 127;									// Transmission Power
	config[3].protocol = PROT_APRS_AFSK;					// Protocol APRS/SSDV (AFSK)
	config[3].frequency.type = FREQ_APRS_REGION;			// Dynamic frequency allocation
	config[3].frequency.hz = 144800000;						// Transmission frequency 144.800 MHz
	config[3].packet_spacing = 20000;						// Packet spacing in ms
	config[3].trigger.type = TRIG_CONTINUOUSLY;				// Transmit continuously
	chsnprintf(config[3].aprs_conf.callsign, 16, "DL7AD");	// APRS Callsign
	config[3].aprs_conf.ssid = 14;							// APRS SSID
	config[3].aprs_conf.preamble = 200;						// APRS Preamble (200ms)
	config[3].ssdv_conf.ram_buffer = ssdv_buffer;			// Camera buffer
	config[3].ssdv_conf.ram_size = sizeof(ssdv_buffer);		// Buffer size
	config[3].ssdv_conf.res = RES_QVGA;						// Resolution QVGA
	config[3].ssdv_conf.redundantTx = true;					// Redundant transmission (transmit packets twice)
	config[3].ssdv_conf.quality = 4;						// Image quality
	//start_image_thread(&config[3]);

	// Module IMAGE, APRS 2m 2GFSK
	config[4].power = 127;									// Transmission Power
	config[4].protocol = PROT_APRS_2GFSK;					// Protocol APRS/SSDV (2GFSK)
	config[4].gfsk_conf.speed = 9600;						// 2GFSK Speed
	config[4].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[4].frequency.hz = 144860000;						// Transmission frequency 144.860 MHz
	config[4].trigger.type = TRIG_CONTINUOUSLY;				// Transmit continuously
	chsnprintf(config[4].aprs_conf.callsign, 16, "DL7AD");	// APRS Callsign
	config[4].aprs_conf.ssid = 13;							// APRS SSID
	config[4].aprs_conf.preamble = 100;						// APRS Preamble (100ms)
	config[4].ssdv_conf.ram_buffer = ssdv_buffer;			// Camera buffer
	config[4].ssdv_conf.ram_size = sizeof(ssdv_buffer);		// Buffer size
	config[4].ssdv_conf.res = RES_VGA;						// Resolution VGA
	config[4].ssdv_conf.quality = 4;						// Image quality
	//start_image_thread(&config[4]);

	// Module IMAGE, SSDV 2m 2FSK
	config[5].power = 127;									// Transmission Power
	config[5].protocol = PROT_SSDV_2FSK;					// Protocol SSDV (2FSK)
	config[5].frequency.type = FREQ_STATIC;					// Static frequency allocation
	config[5].frequency.hz = 144160000;						// Transmission frequency 144.160 MHz
	config[5].trigger.type = TRIG_CONTINUOUSLY;				// Transmit continuously
	config[5].fsk_conf.bits = 8;							// 8bit
	config[5].fsk_conf.stopbits = 2;						// 2 Stopbits
	config[5].fsk_conf.predelay = 3000;						// Preamble (1000ms)
	config[5].fsk_conf.baud = 600;							// Baudrate (600baud)
	config[5].fsk_conf.shift = 425;							// Frequency shift (1000Hz)
	chsnprintf(config[5].ssdv_conf.callsign, 7, "DL7AD");	// SSDV Callsign
	config[5].ssdv_conf.ram_buffer = ssdv_buffer;			// Camera buffer
	config[5].ssdv_conf.ram_size = sizeof(ssdv_buffer);		// Buffer size
	config[5].ssdv_conf.res = RES_QVGA;						// Resolution QVGA
	config[5].ssdv_conf.quality = 4;						// Image quality
	//start_image_thread(&config[5]);


	/* ----------------------------------------------------- LOG TRANSMISSION ---------------------------------------------------- */

	// Module LOG, APRS 2m AFSK
	config[6].power = 127;									// Transmission Power
	config[6].protocol = PROT_APRS_AFSK;					// Protocol APRS (AFSK)
	config[6].frequency.type = FREQ_APRS_REGION;			// Dynamic frequency allocation
	config[6].frequency.hz = 144800000;						// Default frequency 144.800 MHz
	config[6].init_delay = 5000;							// Module startup delay (5 seconds)
	config[6].trigger.type = TRIG_TIMEOUT;					// Periodic cycling (every 180 seconds)
	config[6].trigger.timeout = 60;							// Timeout 60 sec
	chsnprintf(config[6].aprs_conf.callsign, 16, "DL7AD");	// APRS Callsign
	config[6].aprs_conf.ssid = 15;							// APRS SSID
	config[6].aprs_conf.preamble = 200;						// APRS Preamble (200ms)
	start_logging_thread(&config[6]);
}

