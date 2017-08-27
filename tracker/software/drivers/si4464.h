#ifndef __SI4464__H__
#define __SI4464__H__

#include "ch.h"
#include "hal.h"
#include "defines.h"
#include "types.h"

#define RADIO_MOD_GPIO(state)			palWriteLine(LINE_RADIO_GPIO, state)
#define inRadioBand(freq) 				(RADIO_MIN_FREQ <= (freq) && (freq) <= RADIO_MAX_FREQ)

void Si4464_Init(void);
void Si4464_write(uint8_t* txData, uint32_t len);
void setFrequency(uint32_t freq, uint16_t shift);
void setShift(uint16_t shift);
void setModemAFSK(void);
void setModemOOK(void);
void setModem2FSK(void);
void setModem2GFSK(gfsk_conf_t* conf);
void setDeviation(uint32_t deviation);
void setPowerLevel(int8_t level);
void startTx(uint16_t size);
void stopTx(void);
void Si4464_shutdown(void);
bool radioTune(uint32_t frequency, uint16_t shift, int8_t level, uint16_t size);
void Si4464_writeFIFO(uint8_t *msg, uint8_t size);
uint8_t Si4464_freeFIFO(void);
uint8_t Si4464_getState(void);
int8_t Si4464_getTemperature(void);
bool isRadioInitialized(void);

#endif

