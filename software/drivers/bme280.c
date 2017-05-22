/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor
  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650
  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.
  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!
  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include "ch.h"
#include "hal.h"
#include "bme280.h"
#include "pi2c.h"
#include <math.h>

bool BME280_isAvailable(uint8_t address)
{
	uint8_t val;
	if(I2C_read8(address, BME280_REGISTER_CHIPID, &val))
		return val == 0x60;
	else
		return false;
}

/**
  * Initializes BME280 and reads calibration data
  */
void BME280_Init(bme280_t *handle, uint8_t address)
{
	uint8_t tmp1;
	uint8_t tmp2;

	handle->address = address;

	I2C_read16_LE(address, BME280_REGISTER_DIG_T1, &handle->calib.dig_T1);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_T2, &handle->calib.dig_T2);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_T3, &handle->calib.dig_T3);

	I2C_read16_LE(address, BME280_REGISTER_DIG_P1, &handle->calib.dig_P1);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P2, &handle->calib.dig_P2);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P3, &handle->calib.dig_P3);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P4, &handle->calib.dig_P4);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P5, &handle->calib.dig_P5);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P6, &handle->calib.dig_P6);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P7, &handle->calib.dig_P7);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P8, &handle->calib.dig_P8);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_P9, &handle->calib.dig_P9);

	I2C_read8(address, BME280_REGISTER_DIG_H1, &handle->calib.dig_H1);
	I2C_readS16_LE(address, BME280_REGISTER_DIG_H2, &handle->calib.dig_H2);
	I2C_read8(address, BME280_REGISTER_DIG_H3, &handle->calib.dig_H3);

	I2C_read8(address, BME280_REGISTER_DIG_H4, &tmp1);
	I2C_read8(address, BME280_REGISTER_DIG_H5, &tmp2);
	handle->calib.dig_H4 = (((int8_t)tmp1) << 4) | (tmp2 & 0x0F);

	I2C_read8(address, BME280_REGISTER_DIG_H6, &tmp1);
	I2C_read8(address, BME280_REGISTER_DIG_H5, &tmp2);
	handle->calib.dig_H5 = (((int8_t)tmp1) << 4) | (tmp2 >> 4 & 0x0F);

	I2C_read8(address, BME280_REGISTER_DIG_H6, (uint8_t*)&handle->calib.dig_H6);

	I2C_write8(address, BME280_REGISTER_CONTROLHUMID, 0x03); // Set before CONTROL (DS 5.4.3)
	I2C_write8(address, BME280_REGISTER_CONTROL, 0x3F);
	chThdSleepMilliseconds(50); // Wait for BME280

	BME280_getTemperature(handle); // Set t_fine
}

/**
  * Reads the temperature
  * @return Temperature in degC * 100
  */
int16_t BME280_getTemperature(bme280_t *handle)
{
	int32_t var1, var2, adc_T;
	uint16_t tmp;

	I2C_read16(handle->address, BME280_REGISTER_TEMPDATA, &tmp);
	adc_T = tmp;
	I2C_read8(handle->address, BME280_REGISTER_TEMPDATA+2, (uint8_t*)&tmp);
	adc_T <<= 8;
	adc_T |= tmp & 0xFF;
	adc_T >>= 4;

	var1 = ((((adc_T>>3) - ((int32_t)handle->calib.dig_T1 <<1))) * ((int32_t)handle->calib.dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)handle->calib.dig_T1)) * ((adc_T>>4) - ((int32_t)handle->calib.dig_T1))) >> 12) * ((int32_t)handle->calib.dig_T3)) >> 14;

	handle->t_fine = var1 + var2;

	return (handle->t_fine * 5 + 128) >> 8;
}

/**
  * Reads the barometric pressure
  * @param Values to be sampled
  * @return Pressure in Pa * 10
  */
uint32_t BME280_getPressure(bme280_t *handle, uint16_t means) {
	int64_t var1, var2, p;
	uint16_t tmp;

	uint64_t sum = 0;
	for(uint16_t i=0; i<means; i++) {
		int32_t adc_P;
		I2C_read16(handle->address, BME280_REGISTER_PRESSUREDATA, &tmp);
		adc_P = tmp;
		I2C_read8(handle->address, BME280_REGISTER_PRESSUREDATA+2, (uint8_t*)&tmp);
		adc_P <<= 8;
		adc_P |= tmp & 0xFF;
		adc_P >>= 4;

		var1 = ((int64_t)handle->t_fine) - 128000;
		var2 = var1 * var1 * (int64_t)handle->calib.dig_P6;
		var2 = var2 + ((var1*(int64_t)handle->calib.dig_P5)<<17);
		var2 = var2 + (((int64_t)handle->calib.dig_P4)<<35);
		var1 = ((var1 * var1 * (int64_t)handle->calib.dig_P3)>>8) + ((var1 * (int64_t)handle->calib.dig_P2)<<12);
		var1 = (((((int64_t)1)<<47)+var1))*((int64_t)handle->calib.dig_P1)>>33;

		if (var1 == 0)
			return 0;  // avoid exception caused by division by zero

		p = 1048576 - adc_P;
		p = (((p<<31) - var2)*3125) / var1;
		var1 = (((int64_t)handle->calib.dig_P9) * (p>>13) * (p>>13)) >> 25;
		var2 = (((int64_t)handle->calib.dig_P8) * p) >> 19;

		sum += ((p + var1 + var2) >> 8) + (((int64_t)handle->calib.dig_P7)<<4);
	}

	return sum/(means*26);
}

/**
  * Reads the relative humidity
  * @return rel. humidity in % * 10
  */
uint16_t BME280_getHumidity(bme280_t *handle) {
	int32_t adc_H;
	uint16_t tmp;
	I2C_read16(handle->address, BME280_REGISTER_HUMIDDATA, &tmp);
	adc_H = tmp;

	int32_t v_x1_u32r;

	v_x1_u32r = (handle->t_fine - ((int32_t)76800));

	v_x1_u32r = (((((adc_H << 14) - (((int32_t)handle->calib.dig_H4) << 20) -
		(((int32_t)handle->calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
		(((((((v_x1_u32r * ((int32_t)handle->calib.dig_H6)) >> 10) *
		(((v_x1_u32r * ((int32_t)handle->calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
		((int32_t)2097152)) * ((int32_t)handle->calib.dig_H2) + 8192) >> 14));

	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)handle->calib.dig_H1)) >> 4));

	v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
	v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
	float h = (v_x1_u32r>>12);
	return h / 102;
}

/**
  * Calculates the altitude (in meters) from the specified atmospheric
  * pressure (in Pa*10), and sea-level pressure (in Pa).
  * @param seaLevel Sea-level pressure in Pa
  * @param atmospheric Airpressure in Pa*10
  * @return altitude in cm
  */
int32_t BME280_getAltitude(uint32_t seaLevel, uint32_t atmospheric)
{
	return (1.0-pow((float)atmospheric/(float)(seaLevel*10), 1.0/5.255)) * 288150000 / 65;
}
