/**
 * Si4464 driver specialized for APRS transmissions. Modulation concept has been taken
 * from Stefan Biereigel DK3SB.
 * @see http://www.github.com/thasti/utrak
 */

#include "ch.h"
#include "hal.h"
#include "si4464.h"
#include "modules.h"
#include "debug.h"
#include "types.h"
#include <string.h>

static const SPIConfig ls_spicfg1 = {
	.ssport	= PORT(RADIO_CS),
	.sspad	= PIN(RADIO_CS),
	.cr1	= SPI_CR1_MSTR
};
#define getSPIDriver() &ls_spicfg1

uint32_t outdiv;
bool initialized = false;

/**
 * Initializes Si4464 transceiver chip. Adjustes the frequency which is shifted by variable
 * oscillator voltage.
 * @param mv Oscillator voltage in mv
 */
void Si4464_Init(mod_t modulation) {
	// Initialize SPI
	palSetPadMode(PORT(SPI_SCK), PIN(SPI_SCK), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);			// SCK
	palSetPadMode(PORT(SPI_MISO), PIN(SPI_MISO), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);			// MISO
	palSetPadMode(PORT(SPI_MOSI), PIN(SPI_MOSI), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);			// MOSI
	palSetPadMode(PORT(RADIO_CS), PIN(RADIO_CS), PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);	// RADIO CS
	palSetPad(PORT(RADIO_CS), PIN(RADIO_CS));

	// Configure pins
	palSetPadMode(PORT(RADIO_SDN), PIN(RADIO_SDN), PAL_MODE_OUTPUT_PUSHPULL);		// RADIO SDN
	palSetPadMode(PORT(RADIO_GPIO0), PIN(RADIO_GPIO0), PAL_MODE_OUTPUT_PUSHPULL);	// RADIO GPIO0
	palSetPadMode(PORT(RADIO_GPIO1), PIN(RADIO_GPIO1), PAL_MODE_OUTPUT_PUSHPULL);	// RADIO GPIO1

	// Power up transmitter
	RADIO_SDN_SET(false);	// Radio SDN low (power up transmitter)
	chThdSleepMilliseconds(10);		// Wait for transmitter to power up

	// Power up (transmits oscillator type)
	uint8_t x3 = (OSC_FREQ >> 24) & 0x0FF;
	uint8_t x2 = (OSC_FREQ >> 16) & 0x0FF;
	uint8_t x1 = (OSC_FREQ >>  8) & 0x0FF;
	uint8_t x0 = (OSC_FREQ >>  0) & 0x0FF;
	uint8_t init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};
	Si4464_write(init_command, 7);

	// Set transmitter GPIOs
	uint8_t gpio_pin_cfg_command[] = {
		0x13,	// Command type = GPIO settings
		0x44,	// GPIO0        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
		0x00,	// GPIO1        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
		0x00,	// GPIO2        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
		0x00,	// GPIO3        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
		0x00,	// NIRQ
		0x00,	// SDO
		0x00	// GEN_CONFIG
	};
	Si4464_write(gpio_pin_cfg_command, 8);

	// Set modem
	switch(modulation)
	{
		case MOD_AFSK:
			setModemAFSK();
			break;
		case MOD_OOK:
			setModemOOK();
			break;
		case MOD_2FSK:
			setModem2FSK();
			break;
		case MOD_2GFSK:
			setModem2GFSK();
			break;
		case MOD_DOMINOEX16:
			TRACE_WARN("SI   > Unimplemented modulation %s", VAL2MOULATION(modulation)); // TODO: Implement DominoEX16
	}

	// Temperature readout
	TRACE_INFO("SI   > Transmitter temperature %d degC", Si4464_getTemperature());
	initialized = true;
}

void Si4464_write(uint8_t* txData, uint32_t len) {
	// Transmit data by SPI
	uint8_t rxData[len];
	
	// SPI transfer
	spiAcquireBus(&SPID2);
	spiStart(&SPID2, getSPIDriver());
	spiSelect(&SPID2);
	spiExchange(&SPID2, len, txData, rxData);
	spiUnselect(&SPID2);
	spiReleaseBus(&SPID2);

	// Reqest ACK by Si4464
	uint32_t counter = 0; // FIXME: Sometimes CTS is not returned by Si4464 correctly
	rxData[1] = 0x00;
	while(rxData[1] != 0xFF && ++counter < 2000) {

		// Request ACK by Si4464
		uint8_t rx_ready[] = {0x44};

		// SPI transfer
		spiAcquireBus(&SPID2);
		spiStart(&SPID2, getSPIDriver());
		spiSelect(&SPID2);
		spiExchange(&SPID2, 3, rx_ready, rxData);
		spiUnselect(&SPID2);
		spiReleaseBus(&SPID2);
	}
}

/**
 * Read register from Si4464. First Register CTS is included.
 */
void Si4464_read(uint8_t* txData, uint32_t txlen, uint8_t* rxData, uint32_t rxlen) {
	// Transmit data by SPI
	uint8_t null_spi[txlen];
	// SPI transfer
	spiAcquireBus(&SPID2);
	spiStart(&SPID2, getSPIDriver());
	spiSelect(&SPID2);
	spiExchange(&SPID2, txlen, txData, null_spi);
	spiUnselect(&SPID2);
	spiReleaseBus(&SPID2);

	// Reqest ACK by Si4464
	uint32_t counter = 0; // FIXME: Sometimes CTS is not returned by Si4464 correctly
	rxData[1] = 0x00;
	while(rxData[1] != 0xFF && ++counter < 2000) {

		// Request ACK by Si4464
		uint16_t rx_ready[rxlen];
		rx_ready[0] = 0x44;

		// SPI transfer
		spiAcquireBus(&SPID2);
		spiStart(&SPID2, getSPIDriver());
		spiSelect(&SPID2);
		spiExchange(&SPID2, rxlen, rx_ready, rxData);
		spiUnselect(&SPID2);
		spiReleaseBus(&SPID2);
	}
}

void setFrequency(uint32_t freq, uint16_t shift) {
	// Set the output divider according to recommended ranges given in Si4464 datasheet
	uint32_t band = 0;
	if(freq < 705000000UL) {outdiv = 6;  band = 1;};
	if(freq < 525000000UL) {outdiv = 8;  band = 2;};
	if(freq < 353000000UL) {outdiv = 12; band = 3;};
	if(freq < 239000000UL) {outdiv = 16; band = 4;};
	if(freq < 177000000UL) {outdiv = 24; band = 5;};

	// Set the band parameter
	uint32_t sy_sel = 8;
	uint8_t set_band_property_command[] = {0x11, 0x20, 0x01, 0x51, (band + sy_sel)};
	Si4464_write(set_band_property_command, 5);

	// Set the PLL parameters
	uint32_t f_pfd = 2 * OSC_FREQ / outdiv;
	uint32_t n = ((uint32_t)(freq / f_pfd)) - 1;
	float ratio = (float)freq / (float)f_pfd;
	float rest  = ratio - (float)n;

	uint32_t m = (uint32_t)(rest * 524288UL);
	uint32_t m2 = m >> 16;
	uint32_t m1 = (m - m2 * 0x10000) >> 8;
	uint32_t m0 = (m - m2 * 0x10000 - (m1 << 8));

	uint32_t channel_increment = 524288 * outdiv * shift / (2 * OSC_FREQ);
	uint8_t c1 = channel_increment / 0x100;
	uint8_t c0 = channel_increment - (0x100 * c1);

	uint8_t set_frequency_property_command[] = {0x11, 0x40, 0x04, 0x00, n, m2, m1, m0, c1, c0};
	Si4464_write(set_frequency_property_command, 10);

	uint32_t x = ((((uint32_t)1 << 19) * outdiv * 1300.0)/(2*OSC_FREQ))*2;
	uint8_t x2 = (x >> 16) & 0xFF;
	uint8_t x1 = (x >>  8) & 0xFF;
	uint8_t x0 = (x >>  0) & 0xFF;
	uint8_t set_deviation[] = {0x11, 0x20, 0x03, 0x0a, x2, x1, x0};
	Si4464_write(set_deviation, 7);
}

void setShift(uint16_t shift) {
	if(!shift)
		return;

	float units_per_hz = (( 0x40000 * outdiv ) / (float)OSC_FREQ);

	// Set deviation for 2FSK
	uint32_t modem_freq_dev = (uint32_t)(units_per_hz * shift / 2.0 );
	uint8_t modem_freq_dev_0 = 0xFF & modem_freq_dev;
	uint8_t modem_freq_dev_1 = 0xFF & (modem_freq_dev >> 8);
	uint8_t modem_freq_dev_2 = 0xFF & (modem_freq_dev >> 16);

	uint8_t set_modem_freq_dev_command[] = {0x11, 0x20, 0x03, 0x0A, modem_freq_dev_2, modem_freq_dev_1, modem_freq_dev_0};
	Si4464_write(set_modem_freq_dev_command, 7);
}

void setModemAFSK(void) {
	// Disable preamble
	uint8_t disable_preamble[] = {0x11, 0x10, 0x01, 0x00, 0x00};
	Si4464_write(disable_preamble, 5);

	// Do not transmit sync word
	uint8_t no_sync_word[] = {0x11, 0x11, 0x01, 0x00, (0x01 << 7)};
	Si4464_write(no_sync_word, 5);

	// Setup the NCO modulo and oversampling mode
	uint32_t s = OSC_FREQ / 10;
	uint8_t f3 = (s >> 24) & 0xFF;
	uint8_t f2 = (s >> 16) & 0xFF;
	uint8_t f1 = (s >>  8) & 0xFF;
	uint8_t f0 = (s >>  0) & 0xFF;
	uint8_t setup_oversampling[] = {0x11, 0x20, 0x04, 0x06, f3, f2, f1, f0};
	Si4464_write(setup_oversampling, 8);

	// Setup the NCO data rate for APRS
	uint8_t setup_data_rate[] = {0x11, 0x20, 0x03, 0x03, 0x00, 0x11, 0x30};
	Si4464_write(setup_data_rate, 7);

	// Use 2GFSK from async GPIO0
	uint8_t use_2gfsk[] = {0x11, 0x20, 0x01, 0x00, 0x0B};
	Si4464_write(use_2gfsk, 5);

	// Set AFSK filter
	uint8_t coeff[] = {0x81, 0x9f, 0xc4, 0xee, 0x18, 0x3e, 0x5c, 0x70, 0x76};
	uint8_t i;
	for(i=0; i<sizeof(coeff); i++) {
		uint8_t msg[] = {0x11, 0x20, 0x01, 0x17-i, coeff[i]};
		Si4464_write(msg, 5);
	}
}

void setModemOOK(void) {
	// Use OOK from async GPIO0
	uint8_t use_ook[] = {0x11, 0x20, 0x01, 0x00, 0x89};
	Si4464_write(use_ook, 5);
}

void setModem2FSK(void) {
	// use 2FSK from async GPIO0
	uint8_t use_2fsk[] = {0x11, 0x20, 0x01, 0x00, 0x8A};
	Si4464_write(use_2fsk, 5);
}

void setModem2GFSK(void) {
	// Disable preamble
	uint8_t disable_preamble[] = {0x11, 0x10, 0x01, 0x00, 0x00};
	Si4464_write(disable_preamble, 5);

	// Do not transmit sync word
	uint8_t no_sync_word[] = {0x11, 0x11, 0x01, 0x00, (0x01 << 7)};
	Si4464_write(no_sync_word, 5);

	// Setup the NCO modulo and oversampling mode
	uint32_t s = OSC_FREQ / 10;
	uint8_t f3 = (s >> 24) & 0xFF;
	uint8_t f2 = (s >> 16) & 0xFF;
	uint8_t f1 = (s >>  8) & 0xFF;
	uint8_t f0 = (s >>  0) & 0xFF;
	uint8_t setup_oversampling[] = {0x11, 0x20, 0x04, 0x06, f3, f2, f1, f0};
	Si4464_write(setup_oversampling, 8);

	// Setup the NCO data rate for 2GFSK
	uint8_t setup_data_rate[] = {0x11, 0x20, 0x03, 0x03, 0x00, 0x25, 0x80};
	Si4464_write(setup_data_rate, 7);

	// Use 2GFSK from async GPIO0
	uint8_t use_2gfsk[] = {0x11, 0x20, 0x01, 0x00, 0x0B};
	Si4464_write(use_2gfsk, 5);
}

void setPowerLevel(int8_t level) {
	// Set the Power
	uint8_t set_pa_pwr_lvl_property_command[] = {0x11, 0x22, 0x01, 0x01, dBm2powerLvl(level)};
	Si4464_write(set_pa_pwr_lvl_property_command, 5);
}

void startTx(uint16_t size) {
	uint8_t change_state_command[] = {0x31, 0x00, 0x30, (size >> 8) & 0x1F, size & 0xFF};
	Si4464_write(change_state_command, 5);
}

void stopTx(void) {
	uint8_t change_state_command[] = {0x34, 0x03};
	Si4464_write(change_state_command, 2);
}

void radioShutdown(void) {
	RADIO_SDN_SET(true);	// Power down chip
	RF_GPIO1_SET(false);	// Set GPIO1 low
	initialized = false;

	// Deinit pins
	palSetPadMode(PORT(RADIO_CS), PIN(RADIO_CS), PAL_MODE_INPUT);		// RADIO CS
	palSetPadMode(PORT(RADIO_GPIO0), PIN(RADIO_GPIO0), PAL_MODE_INPUT);	// RADIO GPIO0
	palSetPadMode(PORT(RADIO_GPIO1), PIN(RADIO_GPIO1), PAL_MODE_INPUT);	// RADIO GPIO1
}

/**
 * Tunes the radio and activates transmission.
 * @param frequency Transmission frequency in Hz
 * @param shift Shift of FSK in Hz
 * @param level Transmission power level in dBm
 */
bool radioTune(uint32_t frequency, uint16_t shift, int8_t level, uint16_t size) {
	// Tracing
	TRACE_INFO("SI   > Tune Si4464");

	if(!RADIO_WITHIN_FREQ_RANGE(frequency)) {
		TRACE_ERROR("SI   > Frequency out of range");
		TRACE_ERROR("SI   > abort transmission");
		return false;
	}

	setFrequency(frequency, shift);	// Set frequency
	setShift(shift);					// Set shift
	setPowerLevel(level);			// Set power level

	startTx(size);
	return true;
}

void Si4464_writeFIFO(uint8_t *msg, uint8_t size) {
	uint8_t write_fifo[size+1];
	write_fifo[0] = 0x66;
	memcpy(&write_fifo[1], msg, size);
	Si4464_write(write_fifo, size+1);
}

/**
  * Returns free space in FIFO of Si4464
  */
uint8_t Si4464_freeFIFO(void) {
	uint8_t fifo_info[2] = {0x15, 0x00};
	uint8_t rxData[4];
	Si4464_read(fifo_info, 2, rxData, 4);
	return rxData[3];
}


/**
  * Returns internal state of Si4464
  */
uint8_t Si4464_getState(void) {
	uint8_t fifo_info[1] = {0x33};
	uint8_t rxData[4];
	Si4464_read(fifo_info, 1, rxData, 4);
	return rxData[2];
}

int8_t Si4464_getTemperature(void) {
	uint8_t txData[2] = {0x14, 0x10};
	uint8_t rxData[8];
	Si4464_read(txData, 2, rxData, 8);
	uint16_t adc = rxData[7] | ((rxData[6] & 0x7) << 8);
	return (899*adc)/4096 - 293;
}

/**
  * Converts power level from dBm to Si4464 power level. The calculation
  * assumes Vcc = 2.6V and Si4464/Si4463 or Si4063.
  */
uint8_t dBm2powerLvl(int32_t dBm) {
	if(dBm < -35) {
		return 0;
	} else if(dBm < -7) {
		return (uint8_t)((2*dBm+74)/15);
	} else if(dBm < 2) {
		return (uint8_t)((2*dBm+26)/3);
	} else if(dBm < 8) {
		return (uint8_t)((5*dBm+20)/3);
	} else if(dBm < 13) {
		return (uint8_t)(3*dBm-4);
	} else if(dBm < 18) {
		return (uint8_t)((92*dBm-1021)/5);
	} else {
		return 127;
	}
}

bool isRadioInitialized(void) {
	return initialized;
}

