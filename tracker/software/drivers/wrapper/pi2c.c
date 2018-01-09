/**
  * I2C wrapper for ChibiOS due to a bug: I2C blocking when I2C transfer suffered timeout
  * @see https://github.com/psas/stm32/commit/32ec8c97a1e5bf605bd5d41a89fc60b60e136af2
  */
#include "ch.h"
#include "hal.h"
#include "pi2c.h"

#define I2C_DRIVER	(&I2CD1)

static uint8_t error;

const I2CConfig _i2cfg = {
	OPMODE_I2C,
	200000,
	FAST_DUTY_CYCLE_2,
};

static bool I2C_transmit(uint8_t addr, uint8_t *txbuf, uint32_t txbytes, uint8_t *rxbuf, uint32_t rxbytes, systime_t timeout) {
	i2cAcquireBus(I2C_DRIVER);
	i2cStart(I2C_DRIVER, &_i2cfg);
	msg_t i2c_status = i2cMasterTransmitTimeout(I2C_DRIVER, addr, txbuf, txbytes, rxbuf, rxbytes, timeout);
	i2cStop(I2C_DRIVER);
	i2cReleaseBus(I2C_DRIVER);

	if(i2c_status == MSG_TIMEOUT) { // Restart I2C at timeout
		TRACE_ERROR("I2C  > TIMEOUT (ADDR 0x%02x)", addr);
		error = 0x1;
	} else if(i2c_status == MSG_RESET) {
		TRACE_ERROR("I2C  > RESET (ADDR 0x%02x)", addr);
		error = 0x0;
	} else {
		error = 0x0;
	}

	return i2c_status == MSG_OK;
}

void I2C_Lock(void)
{
	i2cAcquireBus(I2C_DRIVER);
}

void I2C_Unlock(void)
{
	i2cReleaseBus(I2C_DRIVER);
}

void pi2cInit(void)
{
	TRACE_INFO("I2C  > Initialize I2C Pins");
	palSetLineMode(LINE_I2C_SDA, PAL_MODE_ALTERNATE(4) | PAL_STM32_OSPEED_HIGHEST | PAL_STM32_OTYPE_OPENDRAIN); // SDA
	palSetLineMode(LINE_I2C_SCL, PAL_MODE_ALTERNATE(4) | PAL_STM32_OSPEED_HIGHEST | PAL_STM32_OTYPE_OPENDRAIN); // SCL
}

bool I2C_write8(uint8_t address, uint8_t reg, uint8_t value)
{
	uint8_t txbuf[] = {reg, value};
	return I2C_transmit(address, txbuf, 2, NULL, 0, MS2ST(100));
}

bool I2C_writeN(uint8_t address, uint8_t *txbuf, uint32_t length)
{
	return I2C_transmit(address, txbuf, length, NULL, 0, MS2ST(100));
}

bool I2C_read8(uint8_t address, uint8_t reg, uint8_t *val)
{
	uint8_t txbuf[] = {reg};
	uint8_t rxbuf[1];
	bool ret = I2C_transmit(address, txbuf, 1, rxbuf, 1, MS2ST(100));
	*val = rxbuf[0];
	return ret;
}

bool I2C_read16(uint8_t address, uint8_t reg, uint16_t *val)
{
	uint8_t txbuf[] = {reg};
	uint8_t rxbuf[2];
	bool ret = I2C_transmit(address, txbuf, 1, rxbuf, 2, MS2ST(100));
	*val =  (rxbuf[0] << 8) | rxbuf[1];
	return ret;
}
bool I2C_read16_LE(uint8_t address, uint8_t reg, uint16_t *val) {
	bool ret = I2C_read16(address, reg, val);
	*val = (*val >> 8) | (*val << 8);
	return ret;
}


bool I2C_read8_16bitreg(uint8_t address, uint16_t reg, uint8_t *val) // 16bit register (for OV5640)
{
	uint8_t txbuf[] = {reg >> 8, reg & 0xFF};
	uint8_t rxbuf[1];
	bool ret = I2C_transmit(address, txbuf, 2, rxbuf, 1, MS2ST(100));
	*val = rxbuf[0];
	return ret;
}

bool I2C_write8_16bitreg(uint8_t address, uint16_t reg, uint8_t value) // 16bit register (for OV5640)
{
	uint8_t txbuf[] = {reg >> 8, reg & 0xFF, value};
	return I2C_transmit(address, txbuf, 3, NULL, 0, MS2ST(100));
}

uint8_t I2C_hasError(void)
{
	return error;
}

