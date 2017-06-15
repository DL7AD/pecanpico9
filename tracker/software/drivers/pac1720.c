#include "ch.h"
#include "hal.h"
#include "debug.h"
#include "pi2c.h"
#include "pac1720.h"
#include "padc.h"

/* 
 * FSP = FSC * FSV
 * FSV = 40 - 40 / Denominator
 * FSC = FSR / R_sense
 */

#define FSR  80		/* Full-scale-rage voltage in mV */
#define DENO 2047	/* Denominator see Tab. 4-5 in PAC1720 datasheet */

#define FSV (40 - 40 / (DENO))
#define FSC ((FSR) / (PAC1720_RSENSE))

static int32_t pac1720_isol;
static int32_t pac1720_pbat;
static int32_t pac1720_isol_counter;
static int32_t pac1720_pbat_counter;

int16_t pac1720_getPbat(void) {
	int32_t fsp = FSV * FSC;
	int16_t val;
	uint8_t sign;

	if(I2C_read16(PAC1720_ADDRESS, PAC1720_CH2_PWR_RAT_HIGH, (uint16_t*)&val)) {
		I2C_read8(PAC1720_ADDRESS, PAC1720_CH2_VSENSE_HIGH, &sign);
		return (sign >> 7 ? 1 : -1) * (val * fsp / 65535);
	} else {
		return 0; // PAC1720 not available (maybe Vcc too low)
	}
}

int16_t pac1720_getIsol(void) {
	if(getUSBVoltageMV() < 300) // USB not connected
	{
		// Short solar cells
		palClearLine(LINE_SOL_SHORT_EN);
		chThdSleepMilliseconds(300); // Wait a little bit to measure a correct value

		int16_t val;
		uint8_t ret = I2C_read16(PAC1720_ADDRESS, PAC1720_CH1_VSENSE_HIGH, (uint16_t*)&val);

		// Unshort solar cells
		palSetLine(LINE_SOL_SHORT_EN);

		// Calculate solar current
		if(ret) {
			val = val < 0 ? 0 : val >> 4; // Current can only flow in one direction (negative values are inaccurate readings)
			return val * FSC / DENO;
		} else {
			return 0; // PAC1720 not available (maybe Vcc too low)
		}
	}
	return 0; // USB connected (we dont want to short USB)
}

int16_t pac1720_getAvgIsol(void) {
	// Return current value if time interval too short
	if(!pac1720_isol_counter)
		pac1720_getIsol();

	// Calculate average power
	int16_t ret = pac1720_isol / pac1720_isol_counter;

	// Reset current measurement
	pac1720_isol = 0;
	pac1720_isol_counter = 0;

	return ret;
}

int16_t pac1720_getAvgPbat(void) {
	// Return current value if time interval too short
	if(!pac1720_pbat_counter)
		pac1720_getPbat();

	// Calculate average power
	int16_t ret = pac1720_pbat / pac1720_pbat_counter;

	// Reset current measurement
	pac1720_pbat = 0;
	pac1720_pbat_counter = 0;

	return ret;
}

uint16_t pac1720_getBatteryVoltage(void) {
	uint16_t val;
	if(!I2C_read16(PAC1720_ADDRESS, PAC1720_CH2_VSOURCE_HIGH, &val))
		return 0; // PAC1720 not available (maybe Vcc too low)

	return (val >> 5) * 20000 / 0x400;
}

uint16_t pac1720_getSolarVoltage(void) {
	uint16_t val;
	if(!I2C_read16(PAC1720_ADDRESS, PAC1720_CH1_VSOURCE_HIGH, &val))
		return 0; // PAC1720 not available (maybe Vcc too low)

	return (val >> 5) * 20000 / 0x400;
}

bool pac1720_isAvailable(void)
{
	uint8_t val;
	if(I2C_read8(PAC1720_ADDRESS, PAC1720_PRODUCT_ID, &val))
		return val == 0x57;
	else
		return false; // PAC1720 not available (maybe Vcc too low)
}

THD_FUNCTION(pac1720_thd, arg)
{
	(void)arg;

	uint8_t cntr = 0;
	while(true)
	{
		// Measure solar short current (if USB not connected)
		if(!cntr++) {
			pac1720_isol += pac1720_getIsol();
			pac1720_isol_counter++;
		}

		// Measure battery power
		pac1720_pbat -= pac1720_getPbat();
		pac1720_pbat_counter++;

		chThdSleepMilliseconds(50);
	}
}

void pac1720_init(void)
{
	TRACE_INFO("PAC  > Init PAC1720");

	// Initialize necessary pins
	palSetLineMode(LINE_SOL_SHORT_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLine(LINE_SOL_SHORT_EN);

	/* Write for both channels
	 * Current sensor sampling time	80ms (Denominator 2047)
	 * Current sensing average enabled 0x3
	 * Current sensing range +-80mV (FSR)
	 */
	I2C_write8(PAC1720_ADDRESS, PAC1720_CH1_VSENSE_SAMP_CONFIG, 0x5F);
	I2C_write8(PAC1720_ADDRESS, PAC1720_CH2_VSENSE_SAMP_CONFIG, 0x5F);
	I2C_write8(PAC1720_ADDRESS, PAC1720_V_SOURCE_SAMP_CONFIG,   0xFF);

	TRACE_INFO("PAC  > Init PAC1720 continuous measurement");
	chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(256), "PAC1720", NORMALPRIO, pac1720_thd, NULL);
}
