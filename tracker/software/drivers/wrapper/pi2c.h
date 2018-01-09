/**
  * I2C wrapper for ChibiOS due to a bug: I2C blocking when I2C transfer suffered timeout
  * @see https://github.com/psas/stm32/commit/32ec8c97a1e5bf605bd5d41a89fc60b60e136af2
  */

#ifndef __I2C_H__
#define __I2C_H__

#include "ch.h"
#include "hal.h"
#include "debug.h"
#include "config.h"

void pi2cInit(void);

bool I2C_write8(uint8_t address, uint8_t reg, uint8_t value);
bool I2C_writeN(uint8_t address, uint8_t *txbuf, uint32_t length);
bool I2C_read8(uint8_t address, uint8_t reg, uint8_t *val);
bool I2C_read16(uint8_t address, uint8_t reg, uint16_t *val);

bool I2C_write8_16bitreg(uint8_t address, uint16_t reg, uint8_t value); // 16bit register (for OV5640)
bool I2C_read8_16bitreg(uint8_t address, uint16_t reg, uint8_t *val); // 16bit register (for OV5640)

bool I2C_read16_LE(uint8_t address, uint8_t reg, uint16_t *val);

void I2C_Lock(void);
void I2C_Unlock(void);

uint8_t I2C_hasError(void);

#endif

