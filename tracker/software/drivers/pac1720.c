#include "ch.h"
#include "hal.h"
#include "debug.h"
#include "pi2c.h"
#include "pac1720.h"
#include "padc.h"
#include <stdlib.h>

/* 
 * FSP = FSC * FSV
 * FSV = 40 - 40 / Denominator
 * FSC = FSR / R_sense
 */

#define FSR  10		/* Full-scale-rage voltage in mV */
#define DENO 2047	/* Denominator see Tab. 4-5 in PAC1720 datasheet */

#define FSV (40 - 40 / (DENO))
#define FSC ((FSR) / (PAC1720_RSENSE))

static int32_t pac1720_pbat;
static int32_t pac1720_psol;
static int32_t pac1720_vbat;
static int32_t pac1720_vsol;
static int32_t pac1720_counter;

static uint8_t error;

static mutex_t mtx;
bool mtx_init = false;

static void pac1720_lock(void) {
	// Initialize mutex
	if(!mtx_init)
		chMtxObjectInit(&mtx);
	mtx_init = true;

	chMtxLock(&mtx);
}

static void pac1720_unlock(void)
{
	chMtxUnlock(&mtx);
}

int16_t pac1720_get_pbat(void) {
	int32_t fsp = FSV * FSC;
	int16_t val;
	uint8_t sign;

	if(I2C_read16(PAC1720_ADDRESS, PAC1720_CH2_PWR_RAT_HIGH, (uint16_t*)&val)) {
		I2C_read8(PAC1720_ADDRESS, PAC1720_CH2_VSENSE_HIGH, &sign);
		return (sign >> 7 ? -1 : 1) * 10 * (val * fsp / 65535);
	} else {
		error |= 0x1;
		return 0; // PAC1720 not available (maybe Vcc too low)
	}
}

int16_t pac1720_get_psol(void) {
	return 0;
}

uint16_t pac1720_get_vbat(void) {
	uint16_t val;
	if(!I2C_read16(PAC1720_ADDRESS, PAC1720_CH2_VSOURCE_HIGH, &val)) {
		error |= 0x1;
		return 0; // PAC1720 not available (maybe Vcc too low)
	}

	uint16_t ret = (val >> 5) * 20000 / 0x400;

	if(ret < 1500)
		error |= 0x2; // The chip is unreliable

	return ret;
}

uint16_t pac1720_get_vsol(void) {
	uint16_t val;
	if(!I2C_read16(PAC1720_ADDRESS, PAC1720_CH1_VSOURCE_HIGH, &val)) {
		error |= 0x1;
		return 0; // PAC1720 not available (maybe Vcc too low)
	}


	return (val >> 5) * 20000 / 0x400;
}

bool pac1720_isAvailable(void)
{
	uint8_t val;
	if(I2C_read8(PAC1720_ADDRESS, PAC1720_PRODUCT_ID, &val)) {
		error |= val != 0x57;
		return val == 0x57;
	} else {
		error |= 0x1;
		return false; // PAC1720 not available (maybe Vcc too low)
	}
}

static void sendConfig(void)
{
	/* Write for both channels
	 * Current sensor sampling time	80ms (Denominator 2047)
	 * Current sensing average enabled 0x3
	 * Current sensing range +-10mV (FSR)
	 */
	I2C_write8(PAC1720_ADDRESS, PAC1720_CH1_VSENSE_SAMP_CONFIG, 0x5C);
	I2C_write8(PAC1720_ADDRESS, PAC1720_CH2_VSENSE_SAMP_CONFIG, 0x5C);
	I2C_write8(PAC1720_ADDRESS, PAC1720_V_SOURCE_SAMP_CONFIG,   0xFF);
}

void pac1720_get_avg(uint16_t* vbat, uint16_t* vsol, int16_t* pbat, int16_t* psol) {
	// Return current value if time interval too short
	if(!pac1720_counter) {
		*vbat = pac1720_get_vbat();
		*vsol = pac1720_get_vsol();
		*pbat = pac1720_get_pbat();
		*psol = pac1720_get_psol();
		return;
	}

	pac1720_lock();

	// Calculate average power
	*vbat = pac1720_vbat / pac1720_counter;
	*vsol = pac1720_vsol / pac1720_counter;
	*pbat = pac1720_pbat / pac1720_counter;
	*psol = pac1720_psol / pac1720_counter;

	// Reset current measurement
	pac1720_vbat = 0;
	pac1720_vsol = 0;
	pac1720_pbat = 0;
	pac1720_psol = 0;
	pac1720_counter = 0;

	pac1720_unlock();
}

THD_FUNCTION(pac1720_thd, arg)
{
	(void)arg;

	while(true)
	{
		// Send config
		sendConfig();

		// Measure battery power
		pac1720_lock();
		pac1720_vbat += pac1720_get_vbat();
		pac1720_vsol += pac1720_get_vsol();
		pac1720_pbat += pac1720_get_pbat();
		pac1720_psol += pac1720_get_psol();
		pac1720_counter++;
		pac1720_unlock();

		chThdSleepMilliseconds(200);
	}
}

void pac1720_init(void)
{
	TRACE_INFO("PAC  > Init PAC1720");

	// Initialize necessary pins
	palSetLineMode(LINE_SOL_SHORT_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLine(LINE_SOL_SHORT_EN);

	// Send config
	sendConfig();

	TRACE_INFO("PAC  > Init PAC1720 continuous measurement");
	chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(512), "PAC1720", LOWPRIO, pac1720_thd, NULL);
	chThdSleepMilliseconds(10);
}

uint8_t pac1720_hasError(void)
{
	uint8_t ret = error;
	error = 0x0;
	return ret;
}

