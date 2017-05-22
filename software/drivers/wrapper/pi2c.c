/**
  * I2C wrapper for ChibiOS due to a bug: I2C blocking when I2C transfer suffered timeout
  * @see https://github.com/psas/stm32/commit/32ec8c97a1e5bf605bd5d41a89fc60b60e136af2
  */
#include "ch.h"
#include "hal.h"
#include "pi2c.h"

const I2CConfig _i2cfg = {
	OPMODE_I2C,
	50000,
	STD_DUTY_CYCLE,
};

mutex_t pi2c_mtx;

bool i2cSendDriver(I2CDriver *driver, uint8_t addr, uint8_t *txbuf, uint32_t txbytes, uint8_t *rxbuf, uint32_t rxbytes, systime_t timeout) {
	i2cAcquireBus(driver);
	msg_t i2c_status = i2cMasterTransmitTimeout(driver, addr, txbuf, txbytes, rxbuf, rxbytes, timeout);
	if(i2c_status == MSG_TIMEOUT) { // Restart I2C at timeout
		TRACE_ERROR("I2C  > TIMEOUT > RESTART (ADDR 0x%02x)", addr);
		i2cStop(driver);
		i2cStart(driver, &_i2cfg);
	} else if(i2c_status == MSG_RESET && addr != 0x4C) { // Prevent PAC1720 to produce message while it doesnt work below 2.5V
		TRACE_ERROR("I2C  > RESET (ADDR 0x%02x)", addr);
	}
	i2cReleaseBus(driver);
	return i2c_status == MSG_OK;
}

bool I2C_send(uint8_t addr, uint8_t *txbuf, uint32_t txbytes, uint8_t *rxbuf, uint32_t rxbytes, systime_t timeout)
{
	return i2cSendDriver(&I2CD1, addr, txbuf, txbytes, rxbuf, rxbytes, timeout);
}

void pi2cInit(void)
{
	TRACE_INFO("I2C  > Initialize I2C");
	i2cStart(&I2CD1, &_i2cfg);
	chMtxObjectInit(&pi2c_mtx);
}

// I2C Mutex locked access functions

bool I2C_write8_locked(uint8_t address, uint8_t reg, uint8_t value)
{
	uint8_t txbuf[] = {reg, value};
	return I2C_send(address, txbuf, 2, NULL, 0, MS2ST(100));
}

bool I2C_writeN_locked(uint8_t address, uint8_t *txbuf, uint32_t length)
{
	return I2C_send(address, txbuf, length, NULL, 0, MS2ST(100));
}

bool I2C_read8_locked(uint8_t address, uint8_t reg, uint8_t *val)
{
	uint8_t txbuf[] = {reg};
	uint8_t rxbuf[1];
	bool ret = I2C_send(address, txbuf, 1, rxbuf, 1, MS2ST(100));
	*val = rxbuf[0];
	return ret;
}

bool I2C_read16_locked(uint8_t address, uint8_t reg, uint16_t *val)
{
	uint8_t txbuf[] = {reg};
	uint8_t rxbuf[2];
	bool ret = I2C_send(address, txbuf, 1, rxbuf, 2, MS2ST(100));
	*val =  (rxbuf[0] << 8) | rxbuf[1];
	return ret;
}

// I2C Mutex unlocked access functions

bool I2C_write8(uint8_t address, uint8_t reg, uint8_t value)
{
	I2C_lock();
	bool ret = I2C_write8_locked(address, reg, value);
	chMtxUnlock(&pi2c_mtx);
	return ret;
}

bool I2C_writeN(uint8_t address, uint8_t *txbuf, uint32_t length)
{
	I2C_lock();
	bool ret = I2C_writeN_locked(address, txbuf, length);
	I2C_unlock();
	return ret;
}

bool I2C_read8(uint8_t address, uint8_t reg, uint8_t *val)
{
	I2C_lock();
	bool ret = I2C_read8_locked(address, reg, val);
	I2C_unlock();
	return ret;
}

bool I2C_read16(uint8_t address, uint8_t reg, uint16_t *val)
{
	I2C_lock();
	bool ret = I2C_read16_locked(address, reg, val);
	I2C_unlock();
	return ret;
}

bool I2C_read16_LE(uint8_t address, uint8_t reg, uint16_t *val) {
	bool ret = I2C_read16_locked(address, reg, val);
	*val = (*val >> 8) | (*val << 8);
	return ret;
}

bool I2C_readS16(uint8_t address, uint8_t reg, int16_t *val)
{
	return I2C_read16(address, reg, (uint16_t*)val);
}

bool I2C_readS16_LE(uint8_t address, uint8_t reg, int16_t* val)
{
	return I2C_read16_LE(address, reg, (uint16_t*)val);
}


/**
  * Locks all other I2C threads to access I2C
  */
void I2C_lock(void)
{
	chMtxLock(&pi2c_mtx);
}

/**
  * Unlocks all other I2C threads to access I2C
  */
void I2C_unlock(void)
{
	chMtxUnlock(&pi2c_mtx);
}

