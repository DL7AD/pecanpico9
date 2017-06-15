#include "ch.h"
#include "hal.h"
#include "defines.h"
#include "config.h"
#include "padc.h"
#include "pac1720.h"
#include "debug.h"

#define ADC_NUM_CHANNELS	4		/* Amount of channels (solar, battery, temperature) */
#define VCC_REF_LOW			1860	/* mV */
#define VCC_REF_HIGH		3070	/* mV */

#define DIVIDER_VSOL		205/64	/* VSol -- 22kOhm -- ADC -- 10kOhm -- GND */
#define DIVIDER_VBAT		205/64	/* VBat -- 22KOhm -- ADC -- 10kOhm -- GND */
#define DIVIDER_VUSB		205/64	/* VUSB -- 22KOhm -- ADC -- 10kOhm -- GND */

static adcsample_t samples[ADC_NUM_CHANNELS]; // ADC sample buffer
uint16_t vcc_ref = VCC_REF_LOW;

void adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n) {
	(void)adcp;
	(void)buffer;
	(void)n;
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 4 samples of 4 channels, SW triggered.
 * Channels:    Solar voltage divider    ADC1_IN9
 *              USB sensor               ADC1_IN14
 *              Battery voltage divider  ADC1_IN13
 *              Temperature sensor       ADC1_IN16
 */
static const ADCConversionGroup adcgrpcfg = {
	FALSE,
	ADC_NUM_CHANNELS,
	adccb,
	NULL,
	/* HW dependent part.*/
	0,
	ADC_CR2_SWSTART,
	ADC_SMPR1_SMP_AN14(ADC_SAMPLE_144) | ADC_SMPR1_SMP_AN13(ADC_SAMPLE_144) | ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_144),
	ADC_SMPR2_SMP_AN9(ADC_SAMPLE_144),
	ADC_SQR1_NUM_CH(ADC_NUM_CHANNELS),
	0,
	ADC_SQR3_SQ1_N(ADC_CHANNEL_IN9) | ADC_SQR3_SQ2_N(ADC_CHANNEL_IN14)  | ADC_SQR3_SQ3_N(ADC_CHANNEL_IN13) | ADC_SQR3_SQ4_N(ADC_CHANNEL_SENSOR)
};

void initADC(void)
{
	adcStart(&ADCD1, NULL);
	adcSTM32EnableTSVREFE();
	palSetLineMode(LINE_ADC_VSOL, PAL_MODE_INPUT_ANALOG); // Solar panels
	palSetLineMode(LINE_ADC_VBAT, PAL_MODE_INPUT_ANALOG); // Battery
	palSetLineMode(LINE_ADC_VUSB, PAL_MODE_INPUT_ANALOG); // USB
}

void deinitADC(void)
{
	adcStop(&ADCD1);
}

void doConversion(void)
{
	initADC();
	adcStartConversion(&ADCD1, &adcgrpcfg, samples, 1);
	chThdSleepMilliseconds(50); // Wait until conversion is finished
	deinitADC();
}

uint16_t getBatteryVoltageMV(void)
{
	doConversion();
	uint16_t vbat = samples[2] * vcc_ref * DIVIDER_VBAT / 4096;

	// Get voltage from PAC1720 (PAC1720 returns false redings below 2.35V)
	if(vbat >= 2400)
	{
		uint16_t vbat_pac = pac1720_getBatteryVoltage(); // Get value from PAC1720
		if(vbat_pac) // Apply it if valid
			vbat = vbat_pac;
	}

	return vbat;
}

uint16_t getSolarVoltageMV(void)
{
	// Get voltage from PAC1720 (PAC1720 returns false redings below 2.35V)
	if(getBatteryVoltageMV() >= 2400)
	{
		uint16_t vsol_pac = pac1720_getSolarVoltage(); // Get value from PAC1720
		if(vsol_pac)
			return vsol_pac;
	}
	return samples[0] * vcc_ref * DIVIDER_VSOL / 4096;
}

uint16_t getUSBVoltageMV(void)
{
	doConversion();
	return samples[1] * vcc_ref * DIVIDER_VUSB / 4096;
}

uint16_t getSTM32Temperature(void)
{
	doConversion();
	return samples[3];
}

void boost_voltage(bool boost)
{
	if(boost)
	{

		// Ramp up to 3.07V
		palClearLine(LINE_VBOOST);
		palSetLineMode(LINE_VBOOST, PAL_MODE_OUTPUT_PUSHPULL);
		palClearLine(LINE_VBOOST);
		vcc_ref = VCC_REF_HIGH;

	} else {

		// Switch back to 1.86V
		palSetLineMode(LINE_VBOOST, PAL_MODE_INPUT);
		vcc_ref = VCC_REF_LOW;

	}
}

