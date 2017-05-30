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

#ifndef __BME280_H__
#define __BME280_H__

#include "ch.h"
#include "hal.h"

#define P_0								101325

#define BME280_ADDRESS_EXT				0x76
#define BME280_ADDRESS_INT				0x77

#define BME280_REGISTER_DIG_T1			0x88
#define BME280_REGISTER_DIG_T2			0x8A
#define BME280_REGISTER_DIG_T3			0x8C

#define BME280_REGISTER_DIG_P1			0x8E
#define BME280_REGISTER_DIG_P2			0x90
#define BME280_REGISTER_DIG_P3			0x92
#define BME280_REGISTER_DIG_P4			0x94
#define BME280_REGISTER_DIG_P5			0x96
#define BME280_REGISTER_DIG_P6			0x98
#define BME280_REGISTER_DIG_P7			0x9A
#define BME280_REGISTER_DIG_P8			0x9C
#define BME280_REGISTER_DIG_P9			0x9E

#define BME280_REGISTER_DIG_H1			0xA1
#define BME280_REGISTER_DIG_H2			0xE1
#define BME280_REGISTER_DIG_H3			0xE3
#define BME280_REGISTER_DIG_H4			0xE4
#define BME280_REGISTER_DIG_H5			0xE5
#define BME280_REGISTER_DIG_H6			0xE7

#define BME280_REGISTER_CHIPID			0xD0
#define BME280_REGISTER_VERSION			0xD1
#define BME280_REGISTER_SOFTRESET		0xE0

#define BME280_REGISTER_CAL26			0xE1

#define BME280_REGISTER_CONTROLHUMID	0xF2
#define BME280_REGISTER_CONTROL			0xF4
#define BME280_REGISTER_CONFIG			0xF5
#define BME280_REGISTER_PRESSUREDATA	0xF7
#define BME280_REGISTER_TEMPDATA		0xFA
#define BME280_REGISTER_HUMIDDATA		0xFD

typedef struct {
	uint16_t dig_T1;
	int16_t  dig_T2;
	int16_t  dig_T3;

	uint16_t dig_P1;
	int16_t  dig_P2;
	int16_t  dig_P3;
	int16_t  dig_P4;
	int16_t  dig_P5;
	int16_t  dig_P6;
	int16_t  dig_P7;
	int16_t  dig_P8;
	int16_t  dig_P9;

	uint8_t  dig_H1;
	int16_t  dig_H2;
	uint8_t  dig_H3;
	int16_t  dig_H4;
	int16_t  dig_H5;
	int8_t   dig_H6;
} bme280_calib_data_t;

// BME280 handle
typedef struct {
	uint8_t address;
	int32_t t_fine;
	bme280_calib_data_t calib;
} bme280_t;

bool BME280_isAvailable(uint8_t address);
void BME280_Init(bme280_t *handle, uint8_t address);
int16_t BME280_getTemperature(bme280_t *handle);
uint32_t BME280_getPressure(bme280_t *handle, uint16_t means);
uint16_t BME280_getHumidity(bme280_t *handle);
int32_t BME280_getAltitude(uint32_t seaLevel, uint32_t atmospheric);

#endif

