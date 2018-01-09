#ifndef __ADC_H__
#define __ADC_H__

#include "ch.h"
#include "hal.h"

#define isUsbConnected()	(getUSBVoltageMV() > 300)

void initADC(void);
void deinitADC(void);
uint16_t stm32_get_vbat(void);
uint16_t stm32_get_vsol(void);
uint16_t stm32_get_vusb(void);
uint16_t stm32_get_temp(void);
void boost_voltage(bool boost);

#endif

