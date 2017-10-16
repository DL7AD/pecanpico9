#include "ch.h"
#include "hal.h"

#include "tracking.h"
#include "debug.h"
#include "radio.h"
#include "si4464.h"
#include "geofence.h"
#include "pi2c.h"
#include "padc.h"
#include <string.h>

// APRS related
#define PLAYBACK_RATE		13200
#define BAUD_RATE			1200									/* APRS AFSK baudrate */
#define SAMPLES_PER_BAUD	(PLAYBACK_RATE / BAUD_RATE)				/* Samples per baud (192kHz / 1200baud = 160samp/baud) */
#define PHASE_DELTA_1200	(((2 * 1200) << 16) / PLAYBACK_RATE)	/* Delta-phase per sample for 1200Hz tone */
#define PHASE_DELTA_2200	(((2 * 2200) << 16) / PLAYBACK_RATE)	/* Delta-phase per sample for 2200Hz tone */

static uint32_t phase_delta;			// 1200/2200 for standard AX.25
static uint32_t phase;					// Fixed point 9.7 (2PI = TABLE_SIZE)
static uint32_t packet_pos;				// Next bit to be sent out
static uint32_t current_sample_in_baud;	// 1 bit = SAMPLES_PER_BAUD samples
static uint8_t current_byte;

// 2FSK related
static uint8_t txs;						// Serial maschine state
static uint8_t txc;						// Current byte
static uint32_t txi;					// Bitcounter of current byte
static uint32_t txj;					// Bytecounter

// Radio related
static mutex_t radio_mtx;				// Radio mutex
bool radio_mtx_init = false;
static mod_t active_mod = MOD_NOT_SET;
static radioMSG_t radio_msg;
static uint8_t radio_buffer[8192];
static uint32_t radio_freq;

static const char *getModulation(uint8_t key) {
	const char *val[] = {"unknown", "OOK", "2FSK", "2GFSK", "AFSK"};
	return val[key];
};

static void initAFSK(void) {
	// Initialize radio
	Si4464_Init();
	setModemAFSK();
	active_mod = MOD_AFSK;
}

uint8_t getAFSKbyte(void)
{
	if(packet_pos == radio_msg.bin_len) 	// Packet transmission finished
		return false;

	uint8_t b = 0;
	for(uint8_t i=0; i<8; i++)
	{
		if(current_sample_in_baud == 0) {
			if((packet_pos & 7) == 0) { // Load up next byte
				current_byte = radio_msg.buffer[packet_pos >> 3];
			} else { // Load up next bit
				current_byte = current_byte / 2;
			}
		}

		// Toggle tone (1200 <> 2200)
		phase_delta = (current_byte & 1) ? PHASE_DELTA_1200 : PHASE_DELTA_2200;

		phase += phase_delta;			// Add delta-phase (delta-phase tone dependent)
		b |= ((phase >> 16) & 1) << i;	// Set modulation bit

		current_sample_in_baud++;

		if(current_sample_in_baud == SAMPLES_PER_BAUD) {	// Old bit consumed, load next bit
			current_sample_in_baud = 0;
			packet_pos++;
		}
	}

	return b;
}

uint8_t ba = HIGH;
uint8_t getFSKbyte(void)
{
	uint8_t b = 0;
	for(uint8_t i=0; i<8; i++)
	{
		switch(txs)
		{
			case 6: // TX-delay
				txj++;
				if(txj > (uint32_t)(radio_msg.fsk_conf->predelay * radio_msg.fsk_conf->baud / 1000)) {
					txj = 0;
					txs = 7;
				}
				break;

			case 7: // Transmit a single char
				if(txj < radio_msg.bin_len/8) {
					txc = radio_msg.buffer[txj]; // Select char
					txj++;
					ba = LOW; // Start Bit (Synchronizing)
					txi = 0;
					txs = 8;
				} else { // Finished to transmit string
					ba = HIGH;
					return b;
				}
				break;

			case 8:
				if(txi < radio_msg.fsk_conf->bits) {
					txi++;
					ba = txc & 1;
					txc = txc >> 1;
				} else {
					ba = HIGH; // Stop Bit
					txi = 0;
					txs = 9;
				}
				break;

			case 9:
				if(radio_msg.fsk_conf->stopbits == 2)
					ba = HIGH; // Stop Bit
				txs = 7;
		}
		b |= ba << i;
	}

	return b;
}

static thread_t *feeder_thd = NULL;
static THD_WORKING_AREA(si_fifo_feeder_wa, 1024);
THD_FUNCTION(si_fifo_feeder_thd2, arg)
{
	(void)arg;

	chRegSetThreadName("radio_tx_feeder");

	// Initialize variables for timer
	phase_delta = PHASE_DELTA_1200;
	phase = 0;
	packet_pos = 0;
	current_sample_in_baud = 0;
	current_byte = 0;
	uint8_t localBuffer[129];
	uint16_t c = 129;
	uint16_t all = (radio_msg.bin_len*SAMPLES_PER_BAUD+7)/8;

	// Initial FIFO fill
	for(uint16_t i=0; i<c; i++)
		localBuffer[i] = getAFSKbyte();
	Si4464_writeFIFO(localBuffer, c);

	// Start transmission
	radioTune(radio_freq, 0, radio_msg.power, all);

	while(c < all) { // Do while bytes not written into FIFO completely
		// Determine free memory in Si4464-FIFO
		uint8_t more = Si4464_freeFIFO();
		if(more > all-c) {
			if((more = all-c) == 0) // Calculate remainder to send
              break; // End if nothing left
		}

		for(uint16_t i=0; i<more; i++)
			localBuffer[i] = getAFSKbyte();

		Si4464_writeFIFO(localBuffer, more); // Write into FIFO
		c += more;
		chThdSleepMilliseconds(15);
	}
	// Shutdown radio (and wait for Si4464 to finish transmission)
	shutdownRadio();

	chThdExit(MSG_OK);
}

THD_FUNCTION(si_fifo_feeder_thd3, arg)
{
	(void)arg;

	chRegSetThreadName("radio_tx_feeder");

	// Initialize variables for timer
	txs = 6;
	txc = 0;
	txi = 0;
	txj = 0;
	uint8_t localBuffer[129];
	uint16_t c = 129;
	uint16_t all = ((radio_msg.fsk_conf->predelay * radio_msg.fsk_conf->baud / 1000) + radio_msg.bin_len + radio_msg.bin_len*(radio_msg.fsk_conf->stopbits+1)/8 + 7) / 8; // FIXME: I transmit more bytes than neccessary

	// Initial FIFO fill
	for(uint16_t i=0; i<c; i++)
		localBuffer[i] = getFSKbyte();
	Si4464_writeFIFO(localBuffer, c);

	// Start transmission
	radioTune(radio_freq, radio_msg.fsk_conf->shift, radio_msg.power, all);

	while(c < all) { // Do while bytes not written into FIFO completely
		// Determine free memory in Si4464-FIFO
		uint8_t more = Si4464_freeFIFO();
		if(more > all-c) {
			if((more = all-c) == 0) // Calculate remainder to send
              break; // End if nothing left
		}

		for(uint16_t i=0; i<more; i++)
			localBuffer[i] = getFSKbyte();

		Si4464_writeFIFO(localBuffer, more); // Write into FIFO
		c += more;
		chThdSleepMilliseconds(500);
	}

	// Shutdown radio (and wait for Si4464 to finish transmission)
	shutdownRadio();

	chThdExit(MSG_OK);
}

static void sendAFSK(void) {
	// Start/re-start FIFO feeder
	feeder_thd = chThdCreateStatic(si_fifo_feeder_wa, sizeof(si_fifo_feeder_wa), HIGHPRIO+1, si_fifo_feeder_thd2, NULL);

	// Wait for the transmitter to start (because it is used as mutex)
	while(Si4464_getState() != SI4464_STATE_TX)
		chThdSleepMilliseconds(1);
}

static void init2GFSK(void) {
	// Initialize radio
	Si4464_Init();
	setModem2GFSK(radio_msg.gfsk_conf);
	active_mod = MOD_2GFSK;
}

THD_FUNCTION(si_fifo_feeder_thd, arg)
{
	(void)arg;

	uint16_t c = 129;
	uint16_t all = (radio_msg.bin_len+7)/8;

	chRegSetThreadName("radio_tx_feeder");
	// Initial FIFO fill
	Si4464_writeFIFO(radio_msg.buffer, c);

	// Start transmission
	radioTune(radio_freq, 0, radio_msg.power, all);

	while(c < all) { // Do while bytes not written into FIFO completely
		// Determine free memory in Si4464-FIFO
		uint8_t more = Si4464_freeFIFO();
		if(more > all-c) {
			if((more = all-c) == 0) // Calculate remainder to send
              break; // End if nothing left
		}
		Si4464_writeFIFO(&radio_msg.buffer[c], more); // Write into FIFO
		c += more;
		chThdSleepMilliseconds(15); // That value is ok up to 38k4
	}

	// Shutdown radio (and wait for Si4464 to finish transmission)
	shutdownRadio();

	chThdExit(MSG_OK);
}

static void send2GFSK(void) {
	// Start/re-start FIFO feeder
	feeder_thd = chThdCreateStatic(si_fifo_feeder_wa, sizeof(si_fifo_feeder_wa), HIGHPRIO+1, si_fifo_feeder_thd, NULL);

	// Wait for the transmitter to start (because it is used as mutex)
	while(Si4464_getState() != SI4464_STATE_TX)
		chThdSleepMilliseconds(1);
}

static void initOOK(void) {
	// Initialize radio
	Si4464_Init();
	setModemOOK(radio_msg.ook_conf);
	active_mod = MOD_OOK;
}

/**
  * Transmits binary OOK message. One bit = 20ms (1: TONE, 0: NO TONE)
  */
static void sendOOK(void) {
	// Start/re-start FIFO feeder
	feeder_thd = chThdCreateStatic(si_fifo_feeder_wa, sizeof(si_fifo_feeder_wa), HIGHPRIO+1, si_fifo_feeder_thd, NULL);

	// Wait for the transmitter to start (because it is used as mutex)
	while(Si4464_getState() != SI4464_STATE_TX)
		chThdSleepMilliseconds(1);
}

static void init2FSK(void) {
	// Initialize radio and tune
	Si4464_Init();
	setModem2FSK(radio_msg.fsk_conf);
	active_mod = MOD_2FSK;
}

static void send2FSK(void) {
	// Start/re-start FIFO feeder
	feeder_thd = chThdCreateStatic(si_fifo_feeder_wa, sizeof(si_fifo_feeder_wa), HIGHPRIO+1, si_fifo_feeder_thd3, NULL);

	// Wait for the transmitter to start (because it is used as mutex)
	while(Si4464_getState() != SI4464_STATE_TX)
		chThdSleepMilliseconds(1);
}

void shutdownRadio(void)
{
	// Wait for PH to finish transmission
	while(Si4464_getState() == SI4464_STATE_TX)
		chThdSleepMilliseconds(10);

	TRACE_INFO("RAD  > Shutdown radio");
	Si4464_shutdown();
	active_mod = MOD_NOT_SET;
}

/**
  * Returns APRS region specific frequency determined by GPS location. It will
  * use the APRS default frequency set in the config file if no GPS fix has
  * been received.
  */
uint32_t getAPRSRegionFrequency(void) {
	trackPoint_t *point = getLastTrackPoint();

	// Position unknown
	if(point == NULL || (point->gps_lat == 0 && point->gps_lon == 0))
		return 0;
	
	// America 144.390 MHz
	if(isPointInAmerica(point->gps_lat, point->gps_lon))
		return APRS_FREQ_AMERICA;

	// China 144.640 MHz
	if(isPointInChina(point->gps_lat, point->gps_lon))
		return APRS_FREQ_CHINA;

	// Japan 144.660 MHz
	if(isPointInJapan(point->gps_lat, point->gps_lon))
		return APRS_FREQ_JAPAN;

	// Southkorea 144.620 MHz
	if(isPointInSouthkorea(point->gps_lat, point->gps_lon))
		return APRS_FREQ_SOUTHKOREA;

	// Southkorea 144.620 MHz
	if(isPointInSoutheastAsia(point->gps_lat, point->gps_lon))
		return APRS_FREQ_SOUTHEASTASIA;

	// Australia 145.175 MHz
	if(isPointInAustralia(point->gps_lat, point->gps_lon))
		return APRS_FREQ_AUSTRALIA;

	// Australia 144.575 MHz
	if(isPointInNewZealand(point->gps_lat, point->gps_lon))
		return APRS_FREQ_NEWZEALAND;

	// Argentina/Paraguay/Uruguay 144.930 MHz
	if(isPointInArgentina(point->gps_lat, point->gps_lon))
		return APRS_FREQ_ARGENTINA;

	// Brazil 145.575 MHz
	if(isPointInBrazil(point->gps_lat, point->gps_lon))
		return APRS_FREQ_BRAZIL;

	// For the rest of the world 144.800 MHz
	return 144800000;
}

/**
  * Sends radio message into message box. This method will return false if message box is full.
  */
bool transmitOnRadio(radioMSG_t *msg, bool shutdown)
{
 	(void)shutdown;
	uint32_t freq = getFrequency(msg->freq); // Get transmission frequency
	if(inRadioBand(freq)) // Frequency in radio radio band
	{
		if(msg->bin_len > 0) // Message length is not zero
		{
			lockRadio(); // Lock radio

			// Copy data
			memcpy(&radio_msg, msg, sizeof(radioMSG_t));
			memcpy(&radio_buffer, msg->buffer, sizeof(radio_buffer));
			radio_msg.buffer = radio_buffer;
			radio_freq = freq;

			TRACE_INFO(	"RAD  > Transmit %d.%03d MHz, Pwr %d, %s, %d bits",
						freq/1000000, (freq%1000000)/1000, msg->power,
						getModulation(msg->mod), msg->bin_len
			);
		
			switch(msg->mod)
			{
				case MOD_2FSK:
					if(active_mod != msg->mod)
						init2FSK();
					send2FSK();
					break;
				case MOD_2GFSK:
					if(active_mod != msg->mod)
						init2GFSK();
					send2GFSK();
					break;
				case MOD_AFSK:
					if(active_mod != msg->mod)
						initAFSK();
					sendAFSK();
					break;
				case MOD_OOK:
					if(active_mod != msg->mod)
						initOOK();
					sendOOK();
					break;
				case MOD_NOT_SET:
					TRACE_ERROR("RAD  > Modulation not set");
					break;
			}

			unlockRadio(); // Unlock radio

		} else {

			TRACE_ERROR("RAD  > It is nonsense to transmit 0 bits, %d.%03d MHz, Pwr dBm, %s, %d bits",
						freq/1000000, (freq%1000000)/1000, msg->power, getModulation(msg->mod), msg->bin_len
			);

		}

	} else { // Frequency out of radio band

		TRACE_ERROR("RAD  > Radio cant transmit on this frequency, %d.%03d MHz, Pwr dBm, %s, %d bits",
					freq/1000000, (freq%1000000)/1000, msg->power, getModulation(msg->mod), msg->bin_len
		);

	}

	return true;
}

uint32_t getFrequency(freq_conf_t *config)
{
	switch(config->type) {
		case FREQ_APRS_REGION:; // Dynamic frequency (determined by GPS position)
			uint32_t freq = getAPRSRegionFrequency();
			if(!freq) // Use default frequency (if freq is 0 = position unknown)
				return config->hz;
			return freq;

		case FREQ_STATIC: // Static frequency
			return config->hz;

		default:
			return 0;
	}
}

void lockRadio(void)
{
	// Initialize mutex
	if(!radio_mtx_init)
		chMtxObjectInit(&radio_mtx);
	radio_mtx_init = true;

	chMtxLock(&radio_mtx);

	// Wait for old feeder thread to terminate
	if(feeder_thd != NULL) // No waiting on first use
		chThdWait(feeder_thd);
}

void unlockRadio(void)
{
	chMtxUnlock(&radio_mtx);
}

