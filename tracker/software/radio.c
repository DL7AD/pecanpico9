#include "ch.h"
#include "hal.h"
#include "defines.h"
#include "debug.h"
#include "radio.h"
#include "si4464.h"
#include "geofence.h"
#include "pi2c.h"
#include "padc.h"
#include <string.h>

#define PLAYBACK_RATE		129000									/* Samples per second (SYSCLK = 45MHz) */
#define BAUD_RATE			1200									/* APRS AFSK baudrate */
#define SAMPLES_PER_BAUD	(PLAYBACK_RATE / BAUD_RATE)				/* Samples per baud */
#define PHASE_DELTA_1200	(((2 * 1200) << 16) / PLAYBACK_RATE)	/* Delta-phase per sample for 1200Hz tone */
#define PHASE_DELTA_2200	(((2 * 2200) << 16) / PLAYBACK_RATE)	/* Delta-phase per sample for 2200Hz tone */

mutex_t radio_mtx;                             // Radio mutex

void initAFSK(radioMSG_t *msg) {
	// Initialize radio and tune
	Si4464_Init(MOD_AFSK);
	radioTune(msg->freq, 0, msg->power, 0);
}

// Initialize variables for AFSK
static uint32_t phase_delta = PHASE_DELTA_1200;	// 1200/2200 for standard AX.25
static uint32_t phase = 0;						// Fixed point 9.7 (2PI = TABLE_SIZE)
static uint32_t packet_pos = 0;					// Next bit to be sent out
static uint32_t current_sample_in_baud = 0;		// 1 bit = SAMPLES_PER_BAUD samples
static uint8_t current_byte = 0;
static radioMSG_t *tim_msg;
static uint32_t gfsk_bit = 0;

void sendAFSK(radioMSG_t *msg) {
	tim_msg = msg;

	phase_delta = PHASE_DELTA_1200;
	phase = 0;
	packet_pos = 0;
	current_sample_in_baud = 0;
	current_byte = 0;

	uint32_t initial_interval = 100; // in timer ticks
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
	nvicEnableVector(TIM7_IRQn, 1/*priority*/);
	TIM7->ARR = initial_interval; /* Timer's period */
	TIM7->PSC = 1;
	TIM7->CR1 &= ~STM32_TIM_CR1_ARPE; /* ARR register is NOT buffered, allows to update timer's period on-fly. */
	TIM7->DIER |= STM32_TIM_DIER_UIE; /* Interrupt enable */
	TIM7->CR1 |= STM32_TIM_CR1_CEN; /* Counter enable */

	// Block execution while timer is running
	while(TIM7->CR1 & STM32_TIM_CR1_CEN)
		chThdSleepMilliseconds(10);
}


/**
  * Fast interrupt handler for AFSK (1200baud) modulation. It has has the
  * highest priority in order to provide an accurate low jitter modulation.
  */
CH_FAST_IRQ_HANDLER(STM32_TIM7_HANDLER)
{
	if(tim_msg->mod == MOD_AFSK) {

		if(packet_pos == tim_msg->bin_len) { // Packet transmission finished
			TIM7->CR1 &= ~STM32_TIM_CR1_CEN;	// Disable timer
			TIM7->SR &= ~STM32_TIM_SR_UIF;		// Reset interrupt flag
			return;
		}

		if(current_sample_in_baud == 0) {
			if((packet_pos & 7) == 0) { // Load up next byte
				current_byte = tim_msg->msg[packet_pos >> 3];
			} else { // Load up next bit
				current_byte = current_byte / 2;
			}
		}

		// Toggle tone (1200 <> 2200)
		phase_delta = (current_byte & 1) ? PHASE_DELTA_1200 : PHASE_DELTA_2200;

		phase += phase_delta;								// Add delta-phase (delta-phase tone dependent)
		MOD_GPIO_SET((phase >> 16) & 1);		// Set modulaton pin (connected to Si4464)

		current_sample_in_baud++;

		if(current_sample_in_baud == SAMPLES_PER_BAUD) {	// Old bit consumed, load next bit
			//palTogglePad(PORT(LED_2YELLOW), PIN(LED_2YELLOW));
			current_sample_in_baud = 0;
			packet_pos++;
		}

	} else if(tim_msg->mod == MOD_2GFSK) {

		if(gfsk_bit >= tim_msg->bin_len) { // Packet transmission finished
			TIM7->CR1 &= ~STM32_TIM_CR1_CEN;	// Disable timer
			TIM7->SR &= ~STM32_TIM_SR_UIF;		// Reset interrupt flag
			return;
		}

		if((gfsk_bit & 7) == 0) { // Load up next byte
			current_byte = tim_msg->msg[gfsk_bit >> 3];
		} else {
			current_byte = current_byte / 2; // Load next bit
		}

		MOD_GPIO_SET(current_byte & 0x1);
		gfsk_bit++;

		//palTogglePad(PORT(LED_2YELLOW), PIN(LED_2YELLOW));

	}

	TIM7->SR &= ~STM32_TIM_SR_UIF;						// Reset interrupt flag
}

void initOOK(radioMSG_t *msg) {
	// Initialize radio and tune
	Si4464_Init(MOD_OOK);
	radioTune(msg->freq, 0, msg->power, 0);
}

/**
  * Transmits binary OOK message. One bit = 20ms (1: TONE, 0: NO TONE)
  */
void sendOOK(radioMSG_t *msg) {
	// Transmit data
	uint32_t bit = 0;
	systime_t time = chVTGetSystemTimeX();
	while(bit < msg->bin_len) {
		MOD_GPIO_SET((msg->msg[bit/8] >> (bit%8)) & 0x1);
		bit++;

		time = chThdSleepUntilWindowed(time, time + MS2ST(1200 / msg->ook_config->speed));
	}
}

// Transmit data (Software UART)
static uint8_t txs;			// Serial maschine state
static uint8_t txc;			// Current byte
static uint32_t txi;		// Bitcounter of current byte
static uint32_t txj;		// Bytecounter
static radioMSG_t *fsk_msg;	// Current message
static virtual_timer_t vt;

static void serial_cb(void *arg) {
	(void)arg;

	switch(txs)
	{
		case 6: // TX-delay
			txj++;
			if(txj > (uint32_t)(fsk_msg->fsk_config->predelay * fsk_msg->fsk_config->baud / 1000)) {
				txj = 0;
				txs = 7;
			}
			break;

		case 7: // Transmit a single char
			if(txj < fsk_msg->bin_len/8) {
				txc = fsk_msg->msg[txj]; // Select char
				txj++;
				MOD_GPIO_SET(LOW); // Start Bit (Synchronizing)
				txi = 0;
				txs = 8;
			} else {
				txj = 0;
				txs = 0; // Finished to transmit string
				MOD_GPIO_SET(HIGH);
			}
			break;

		case 8:
			if(txi < fsk_msg->fsk_config->bits) {
				txi++;
				MOD_GPIO_SET(txc & 1);
				txc = txc >> 1;
			} else {
				MOD_GPIO_SET(HIGH); // Stop Bit
				txi = 0;
				txs = 9;
			}
			break;

		case 9:
			if(fsk_msg->fsk_config->stopbits == 2)
				MOD_GPIO_SET(HIGH); // Stop Bit
			txs = 7;
	}

	// Reload timer
	if(txs) {
		chSysLockFromISR();
		uint32_t delay = US2ST(1000000/fsk_msg->fsk_config->baud);
		chVTSetI(&vt, delay, serial_cb, NULL);
		chSysUnlockFromISR();
	}
}

void init2FSK(radioMSG_t *msg) {
	// Initialize virtual timer
	chVTObjectInit(&vt);

	// Initialize radio and tune
	Si4464_Init(MOD_2FSK);
	MOD_GPIO_SET(HIGH);
	radioTune(msg->freq, msg->fsk_config->shift, msg->power, 0);
}

void send2FSK(radioMSG_t *msg) {
	// Prepare serial machine states
	txs = 6;
	txc = 0;
	txi = 0;
	txj = 0;
	fsk_msg = msg;

	// Modulate
	chVTSet(&vt, 1, serial_cb, NULL);	// Start timer
	while(txs)
		chThdSleepMilliseconds(1);		// Wait for routine to finish
}

void send2GFSK(radioMSG_t *msg) {
	tim_msg = msg;
	gfsk_bit = 0;
	current_byte = 0;

	// Initialize radio and tune
	Si4464_Init(MOD_2GFSK);
	radioTune(msg->freq, 0, msg->power, 0);
	chThdSleepMilliseconds(30);

	uint32_t initial_interval = 1355; // in timer ticks
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
	nvicEnableVector(TIM7_IRQn, 1/*priority*/);
	TIM7->ARR = initial_interval; /* Timer's period */
	TIM7->PSC = 1;
	TIM7->CR1 &= ~STM32_TIM_CR1_ARPE; /* ARR register is NOT buffered, allows to update timer's period on-fly. */
	TIM7->DIER |= STM32_TIM_DIER_UIE; /* Interrupt enable */
	TIM7->CR1 |= STM32_TIM_CR1_CEN; /* Counter enable */

	// Block execution while timer is running
	while(TIM7->CR1 & STM32_TIM_CR1_CEN)
		chThdSleepMilliseconds(10);
}

/**
  * Returns APRS region specific frequency determined by GPS location. It will
  * use the APRS default frequency set in the config file if no GPS fix has
  * been received.
  */
uint32_t getAPRSRegionFrequency2m(void) {
	trackPoint_t *point = getLastTrackPoint();

	// Use this frequency for the rest of the world (unset regions, 144.800 MHz)
	uint32_t freq = APRS_FREQ_OTHER;

	// Position unknown
	if(!point->gps_lat && !point->gps_lon)
		freq = 0; // Use default frequency set in config file
	
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
uint32_t getAPRSRegionFrequency70cm(void) {
	return 432500000;
}
uint32_t getAPRSISSFrequency(void) {
	return 145825000;
}

/**
  * Sends radio message into message box. This method will return false if message box is full.
  */
bool transmitOnRadio(radioMSG_t *msg) {
	// Lock radio
	chMtxLock(&radio_mtx);

	if(inRadioBand(msg->freq)) { // Radio found

		// Lock interference mutex
		chMtxLock(&interference_mtx);

		TRACE_INFO(	"RAD  > Transmit %d.%03d MHz, %d dBm (%d), %s, %d bits",
					msg->freq/1000000, (msg->freq%1000000)/1000, msg->power,
					dBm2powerLvl(msg->power), VAL2MOULATION(msg->mod), msg->bin_len
		);

		#if RADIO_BOOST
		// Switch voltage to 3.2V for transmission (increases output power by ~6dB)
		boost_voltage(HIGH);
		#endif
		
		switch(msg->mod) {
			case MOD_2FSK:
				if(!isRadioInitialized())
					init2FSK(msg);
				send2FSK(msg);
				break;
			case MOD_2GFSK:
				send2GFSK(msg);
				break;
			case MOD_AFSK:
				if(!isRadioInitialized())
					initAFSK(msg);
				sendAFSK(msg);
				break;
			case MOD_OOK:
				if(!isRadioInitialized())
					initOOK(msg);
				sendOOK(msg);
				break;
			case MOD_DOMINOEX16:
				TRACE_ERROR("RAD  > Unimplemented modulation DominoEX16"); // TODO: Implement this
				break;
		}

		radioShutdown(); // Shutdown radio for reinitialization

		#if RADIO_BOOST
		// Switch voltage back to 1.85V
		boost_voltage(LOW);
		#endif

		chMtxUnlock(&interference_mtx); // Heavy interference finished (HF)

	} else { // Error

		TRACE_ERROR("RAD  > No radio available for this frequency, %d.%03d MHz, %d dBm (%d), %s, %d bits",
					msg->freq/1000000, (msg->freq%1000000)/1000, msg->power,
					dBm2powerLvl(msg->power), VAL2MOULATION(msg->mod), msg->bin_len
		);

	}

	// Unlock radio
	chMtxUnlock(&radio_mtx);

	return true;
}

uint32_t getFrequency(freuquency_config_t *config)
{
	uint32_t (*fptr)(void);

	switch(config->type) {
		case FREQ_DYNAMIC: // Dynamic frequency determination
			fptr = config->method;
			uint32_t ret = (*fptr)();
			if(!ret) // Use default frequency
				return config->hz;
			return ret;

		case FREQ_STATIC: // Static frequency
			return config->hz;

		default:
			return 0;
	}
}

