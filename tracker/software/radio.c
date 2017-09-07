#include "ch.h"
#include "hal.h"

#include "defines.h"
#include "tracking.h"
#include "debug.h"
#include "radio.h"
#include "si4464.h"
#include "geofence.h"
#include "pi2c.h"
#include "padc.h"
#include <string.h>

#define PLAYBACK_RATE		((STM32_PCLK1) / 500)					/* Samples per second (48Mhz / 250 = 192kHz) */
#define BAUD_RATE			1200									/* APRS AFSK baudrate */
#define SAMPLES_PER_BAUD	(PLAYBACK_RATE / BAUD_RATE)				/* Samples per baud (192kHz / 1200baud = 160samp/baud) */
#define PHASE_DELTA_1200	(((2 * 1200) << 16) / PLAYBACK_RATE)	/* Delta-phase per sample for 1200Hz tone */
#define PHASE_DELTA_2200	(((2 * 2200) << 16) / PLAYBACK_RATE)	/* Delta-phase per sample for 2200Hz tone */

mutex_t radio_mtx;						// Radio mutex
mod_t active_mod = MOD_NOT_SET;
static uint32_t phase_delta;			// 1200/2200 for standard AX.25
static uint32_t phase;					// Fixed point 9.7 (2PI = TABLE_SIZE)
static uint32_t packet_pos;				// Next bit to be sent out
static uint32_t current_sample_in_baud;	// 1 bit = SAMPLES_PER_BAUD samples
static uint8_t current_byte;
static radioMSG_t tim_msg;

static uint8_t txs;						// Serial maschine state
static uint8_t txc;						// Current byte
static uint32_t txi;					// Bitcounter of current byte
static uint32_t txj;					// Bytecounter

static const char *getModulation(uint8_t key) {
	const char *val[] = {"unknown", "OOK", "2FSK", "2GFSK", "AFSK"};
	return val[key];
};

void initAFSK(radioMSG_t *msg) {
  (void)msg;
	// Initialize radio
	Si4464_Init();
	setModemAFSK();
	active_mod = MOD_AFSK;
}

void sendAFSK(radioMSG_t *msg) {
  (void)msg;
	// Initialize variables for timer
	memcpy(&tim_msg, msg, sizeof(radioMSG_t));

	phase_delta = PHASE_DELTA_1200;
	phase = 0;
	packet_pos = 0;
	current_sample_in_baud = 0;
	current_byte = 0;

	// Tune
	radioTune(msg->freq, 0, msg->power, 0);

	// Initialize timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
	nvicEnableVector(TIM7_IRQn, 1);
	TIM7->ARR = 500;
	TIM7->CR1 &= ~STM32_TIM_CR1_ARPE;
	TIM7->DIER |= STM32_TIM_DIER_UIE;

	// Start timer
	TIM7->CR1 |= STM32_TIM_CR1_CEN;

	// Block execution while timer is running
	while(TIM7->CR1 & STM32_TIM_CR1_CEN)
		chThdSleepMilliseconds(10);

	shutdownRadio();
}

void init2GFSK(radioMSG_t *msg) {
	// Initialize radio
	Si4464_Init();
	setModem2GFSK(msg->gfsk_conf);
	active_mod = MOD_2GFSK;
}

thread_t *feeder_thd = NULL;

/*
 * TODO: I'd suggest re-working the FIFO handler system.
 * Will give it some thought.
 * Meanwhile there are a few changes that are worth testing.
 */
static THD_WORKING_AREA(si_fifo_feeder_wa, 1024);
THD_FUNCTION(si_fifo_feeder_thd, arg)
{
	(void)arg;

	uint16_t c = 64;
	uint16_t all = (tim_msg.bin_len+7)/8;

	chRegSetThreadName("radio_tx_feeder");
	// Initial FIFO fill
	Si4464_writeFIFO(tim_msg.msg, c);

	// Start transmission
	radioTune(tim_msg.freq, 0, tim_msg.power, all);

	while(c < all) { // Do while bytes not written into FIFO completely
		//chThdSuspendS(&feeder_ref); // Suspend until interrupt resumes it

		// Determine free memory in Si4464-FIFO
		uint8_t more = Si4464_freeFIFO();
		if(more > all-c) {
			if((more = all-c) == 0) // Calculate remainder to send
              break; // End if nothing left
		}
		Si4464_writeFIFO(&tim_msg.msg[c], more); // Write into FIFO
		c += more;
		chThdSleepMilliseconds(5);
	}

	// Shutdown radio (and wait for Si4464 to finish transmission)
	shutdownRadio();

	chThdExit(MSG_OK);
}

void send2GFSK(radioMSG_t *msg) {
	// Copy data
	memcpy(&tim_msg, msg, sizeof(radioMSG_t));

	// Start/re-start FIFO feeder
	feeder_thd = chThdCreateStatic(si_fifo_feeder_wa, sizeof(si_fifo_feeder_wa), HIGHPRIO+1, si_fifo_feeder_thd, NULL);

	// Wait for the transmitter to start (because it is used as mutex)
	while(Si4464_getState() != SI4464_STATE_TX)
		chThdSleepMilliseconds(1);
}

/**
  * Fast interrupt handler for AFSK modulation. It has has the highest priority
  * in order to provide an accurate low jitter modulation.
  */
CH_FAST_IRQ_HANDLER(STM32_TIM7_HANDLER)
{
	if(active_mod == MOD_AFSK) // AFSK
	{

		if(packet_pos == tim_msg.bin_len) { 	// Packet transmission finished
			TIM7->CR1 &= ~STM32_TIM_CR1_CEN;	// Disable timer
			TIM7->SR &= ~STM32_TIM_SR_UIF;		// Reset interrupt flag
			return;
		}

		if(current_sample_in_baud == 0) {
			if((packet_pos & 7) == 0) { // Load up next byte
				current_byte = tim_msg.msg[packet_pos >> 3];
			} else { // Load up next bit
				current_byte = current_byte / 2;
			}
		}

		// Toggle tone (1200 <> 2200)
		phase_delta = (current_byte & 1) ? PHASE_DELTA_1200 : PHASE_DELTA_2200;

		phase += phase_delta;					// Add delta-phase (delta-phase tone dependent)
		RADIO_WRITE_GPIO((phase >> 16) & 1);	// Set modulaton pin (connected to Si4464)

		current_sample_in_baud++;

		if(current_sample_in_baud == SAMPLES_PER_BAUD) {	// Old bit consumed, load next bit
			current_sample_in_baud = 0;
			packet_pos++;
		}

	} else { // 2FSK

		switch(txs)
		{
			case 6: // TX-delay
				txj++;
				if(txj > (uint32_t)(tim_msg.fsk_conf->predelay * tim_msg.fsk_conf->baud / 1000)) {
					txj = 0;
					txs = 7;
				}
				break;

			case 7: // Transmit a single char
				if(txj < tim_msg.bin_len/8) {
					txc = tim_msg.msg[txj]; // Select char
					txj++;
					RADIO_WRITE_GPIO(LOW); // Start Bit (Synchronizing)
					txi = 0;
					txs = 8;
				} else { // Finished to transmit string
					RADIO_WRITE_GPIO(HIGH);
					TIM7->CR1 &= ~STM32_TIM_CR1_CEN; // Disable timer
					TIM7->SR &= ~STM32_TIM_SR_UIF; // Reset interrupt flag
					return;
				}
				break;

			case 8:
				if(txi < tim_msg.fsk_conf->bits) {
					txi++;
					RADIO_WRITE_GPIO(txc & 1);
					txc = txc >> 1;
				} else {
					RADIO_WRITE_GPIO(HIGH); // Stop Bit
					txi = 0;
					txs = 9;
				}
				break;

			case 9:
				if(tim_msg.fsk_conf->stopbits == 2)
					RADIO_WRITE_GPIO(HIGH); // Stop Bit
				txs = 7;
		}

	}

	palToggleLine(LINE_IO_LED1);

	TIM7->SR &= ~STM32_TIM_SR_UIF; // Reset interrupt flag
}

void initOOK(radioMSG_t *msg) {
  (void)msg;
	// Initialize radio
	Si4464_Init();
	setModemOOK();
	active_mod = MOD_OOK;
}

/**
  * Transmits binary OOK message. One bit = 20ms (1: TONE, 0: NO TONE)
  */
void sendOOK(radioMSG_t *msg) {
	// Tune
	radioTune(msg->freq, 0, msg->power, 0);

	// Transmit data
	uint32_t bit = 0;
	systime_t time = chVTGetSystemTimeX();
	while(bit < msg->bin_len) {
		RADIO_WRITE_GPIO((msg->msg[bit/8] >> (bit%8)) & 0x1);
		bit++;

		time = chThdSleepUntilWindowed(time, time + MS2ST(1200 / msg->ook_conf->speed));
	}
	shutdownRadio();
}

void init2FSK(radioMSG_t *msg) {
  (void)msg;
	// Initialize radio and tune
	Si4464_Init();
	setModem2FSK();
}

void send2FSK(radioMSG_t *msg) {
	// Initialize variables for timer
	memcpy(&tim_msg, msg, sizeof(radioMSG_t));

	txs = 6;
	txc = 0;
	txi = 0;
	txj = 0;

	// Tune
	radioTune(msg->freq, msg->fsk_conf->shift, msg->power, 0);

	// Initialize timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
	nvicEnableVector(TIM7_IRQn, 1);
	TIM7->ARR = STM32_PCLK1 / 16 / msg->fsk_conf->baud; // FIXME: 5625000 should be actually STM32_PCLK1
	TIM7->PSC = 15;
	TIM7->CR1 &= ~STM32_TIM_CR1_ARPE;
	TIM7->DIER |= STM32_TIM_DIER_UIE;

	// Start timer
	TIM7->CR1 |= STM32_TIM_CR1_CEN;

	// Block execution while timer is running
	while(TIM7->CR1 & STM32_TIM_CR1_CEN)
		chThdSleepMilliseconds(10);

	shutdownRadio();
}

void shutdownRadio(void)
{
	// Wait for PH to finish transmission for 2GFSK
	while(active_mod == MOD_2GFSK && Si4464_getState() == SI4464_STATE_TX)
		chThdSleepMilliseconds(5);

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

	uint32_t freq = 0; // Position unknown
	
	// America 144.390 MHz
	if(isPointInAmerica(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_AMERICA;

	// China 144.640 MHz
	if(isPointInChina(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_CHINA;

	// Japan 144.660 MHz
	if(isPointInJapan(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_JAPAN;

	// Southkorea 144.620 MHz
	if(isPointInSouthkorea(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_SOUTHKOREA;

	// Southkorea 144.620 MHz
	if(isPointInSoutheastAsia(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_SOUTHEASTASIA;

	// Australia 145.175 MHz
	if(isPointInAustralia(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_AUSTRALIA;

	// Australia 144.575 MHz
	if(isPointInNewZealand(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_NEWZEALAND;

	// Argentina/Paraguay/Uruguay 144.930 MHz
	if(isPointInArgentina(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_ARGENTINA;

	// Brazil 145.575 MHz
	if(isPointInBrazil(point->gps_lat, point->gps_lon))
		freq = APRS_FREQ_BRAZIL;

	return freq;
}

/**
  * Sends radio message into message box. This method will return false if message box is full.
  */
bool transmitOnRadio(radioMSG_t *msg, bool shutdown)
{
 	(void)shutdown;
	if(inRadioBand(msg->freq)) // Frequency in radio radio band
	{
		if(inRadioBand(msg->freq)) // Frequency in radio radio band
		{
			lockRadio(); // Lock radio

			TRACE_INFO(	"RAD  > Transmit %d.%03d MHz, Pwr %d, %s, %d bits",
						msg->freq/1000000, (msg->freq%1000000)/1000, msg->power,
						getModulation(msg->mod), msg->bin_len
			);
		
			switch(msg->mod)
			{
				case MOD_2FSK:
					if(active_mod != msg->mod)
						init2FSK(msg);
					send2FSK(msg);
					break;
				case MOD_2GFSK:
					if(active_mod != msg->mod)
						init2GFSK(msg);
					send2GFSK(msg);
					break;
				case MOD_AFSK:
					if(active_mod != msg->mod)
						initAFSK(msg);
					sendAFSK(msg);
					break;
				case MOD_OOK:
					if(active_mod != msg->mod)
						initOOK(msg);
					sendOOK(msg);
					break;
				case MOD_NOT_SET:
					TRACE_ERROR("RAD  > Modulation not set");
					break;
			}
			/*
			 * FIXME: send2GFSK will return as soon as the radio enter TX state.
			 * So the radio lock will be removed and then ssdv analysis can run.
			 * If ssdv analysis finishes before TX is done then tim.buf gets clobbered
			 */
			unlockRadio(); // Unlock radio

		} else {

			TRACE_ERROR("RAD  > It is nonsense to transmit 0 bits, %d.%03d MHz, Pwr dBm, %s, %d bits",
						msg->freq/1000000, (msg->freq%1000000)/1000, msg->power, getModulation(msg->mod), msg->bin_len
			);

		}

	} else { // Frequency out of radio band

		TRACE_ERROR("RAD  > Radio cant transmit on this frequency, %d.%03d MHz, Pwr dBm, %s, %d bits",
					msg->freq/1000000, (msg->freq%1000000)/1000, msg->power, getModulation(msg->mod), msg->bin_len
		);

	}

	return true;
}

uint32_t getFrequency(freq_conf_t *config)
{
	switch(config->type) {
		case FREQ_APRS_REGION:; // Dynamic frequency (determined by GPS position)
			uint32_t freq = getAPRSRegionFrequency();
			if(!freq) // Use default frequency (if freq is not set = position unknown)
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
	chMtxLock(&radio_mtx);

	// Wait for old feeder thread to terminate
	if(feeder_thd != NULL) // No waiting on first use
		chThdWait(feeder_thd);
}

void unlockRadio(void)
{
	chMtxUnlock(&radio_mtx);
}

