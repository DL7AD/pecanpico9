#ifndef __SI4464__H__
#define __SI4464__H__

#include "ch.h"
#include "hal.h"
#include "defines.h"
#include "types.h"

#define RADIO_SDN_SET(state)			palWritePad(PORT(RADIO_SDN), PIN(RADIO_SDN), state)
#define RADIO_CS_SET(state)				palWritePad(PORT(RADIO_CS), PIN(RADIO_CS), state)
#define RF_GPIO0_SET(state)				palWritePad(PORT(RADIO_GPIO0), PIN(RADIO_GPIO0), state)
#define RF_GPIO1_SET(state)				palWritePad(PORT(RADIO_GPIO1), PIN(RADIO_GPIO1), state)
#define MOD_GPIO_SET(state)				RF_GPIO0_SET(state)
#define RADIO_WITHIN_FREQ_RANGE(frequ)	((frequ) >= 119000000 && (frequ) <= 1050000000)

#define inRadioBand(freq) 				(RADIO_MIN_FREQ <= (freq) && (freq) <= RADIO_MAX_FREQ)

void Si4464_Init(mod_t modulation);
void Si4464_write(uint8_t* txData, uint32_t len);
void setFrequency(uint32_t freq, uint16_t shift);
void setShift(uint16_t shift);
void setModemAFSK(void);
void setModemOOK(void);
void setModem2FSK(void);
void setModem2GFSK(void);
void setDeviation(uint32_t deviation);
void setPowerLevel(int8_t level);
void startTx(uint16_t size);
void stopTx(void);
void radioShutdown(void);
bool radioTune(uint32_t frequency, uint16_t shift, int8_t level, uint16_t size);
void Si4464_writeFIFO(uint8_t *msg, uint8_t size);
uint8_t Si4464_freeFIFO(void);
uint8_t Si4464_getState(void);
int8_t Si4464_getTemperature(void);
uint8_t dBm2powerLvl(int32_t dBm);
bool isRadioInitialized(void);

#endif

