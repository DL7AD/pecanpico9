#ifndef __PAC1720__H__
#define __PAC1720__H__

#include "ch.h"
#include "hal.h"

#define PAC1720_RSENSE					0.1 /* Ohm (current sense resistor) */

#define PAC1720_ADDRESS					0x4C

#define PAC1720_CONFIG					0x00
#define PAC1720_CONVERSION_RATE			0x01
#define PAC1720_ONE_SHOT				0x02
#define PAC1720_CH_MASK_REG				0x03
#define PAC1720_HIGH_LIMIT_STATUS		0x04
#define PAC1720_LOW_LIMIT_STATUS		0x05

#define PAC1720_V_SOURCE_SAMP_CONFIG	0x0A

#define PAC1720_CH1_VSENSE_SAMP_CONFIG	0x0B
#define PAC1720_CH2_VSENSE_SAMP_CONFIG	0x0C
#define PAC1720_CH1_VSENSE_HIGH			0x0D
#define PAC1720_CH1_VSENSE_LOW			0x0E
#define PAC1720_CH2_VSENSE_HIGH			0x0F
#define PAC1720_CH2_VSENSE_LOW			0x10

#define PAC1720_CH1_VSOURCE_HIGH		0x11
#define PAC1720_CH1_VSOURCE_LOW			0x12
#define PAC1720_CH2_VSOURCE_HIGH		0x13
#define PAC1720_CH2_VSOURCE_LOW			0x14

#define PAC1720_CH1_PWR_RAT_HIGH		0x15
#define PAC1720_CH1_PWR_RAT_LOW			0x16
#define PAC1720_CH2_PWR_RAT_HIGH		0x17
#define PAC1720_CH2_PWR_RAT_LOW			0x18

#define PAC1720_CH1_VSENSE_LIMIT_HIGH	0x19
#define PAC1720_CH2_VSENSE_LIMIT_HIGH	0x1A
#define PAC1720_CH1_VSENSE_LIMIT_LOW	0x1B
#define PAC1720_CH2_VSENSE_LIMIT_LOW	0x1C

#define PAC1720_CH1_VSOURCE_LIMIT_HIGH	0x1D
#define PAC1720_CH2_VSOURCE_LIMIT_HIGH	0x1E
#define PAC1720_CH1_VSOURCE_LIMIT_LOW	0x1F
#define PAC1720_CH2_VSOURCE_LIMIT_LOW	0x20

#define PAC1720_PRODUCT_ID				0xFD
#define PAC1720_MANUFACTURER_ID			0xFE
#define PAC1720_REVISION				0xFF


int16_t pac1720_get_pbat(void);
int16_t pac1720_get_psol(void);
uint16_t pac1720_get_vbat(void);
uint16_t pac1720_get_vsol(void);
bool pac1720_isAvailable(void);
void pac1720_get_avg(uint16_t* vbat, uint16_t* vsol, int16_t* pbat, int16_t* psol);
void pac1720_init(void);
uint8_t pac1720_hasError(void);

#endif

