#ifndef __RADIO_H__
#define __RADIO_H__

#include "ch.h"
#include "hal.h"
#include "config.h"
#include "si4464.h"
#include "threads.h"

// APRS region frequencies
#define APRS_FREQ_OTHER				144800000
#define APRS_FREQ_AMERICA			144390000
#define APRS_FREQ_CHINA				144640000
#define APRS_FREQ_JAPAN				144660000
#define APRS_FREQ_SOUTHKOREA		144620000
#define APRS_FREQ_SOUTHEASTASIA		144390000
#define APRS_FREQ_AUSTRALIA			145175000
#define APRS_FREQ_NEWZEALAND		144575000
#define APRS_FREQ_ARGENTINA			144930000
#define APRS_FREQ_BRAZIL			145575000

bool transmitOnRadio(radioMSG_t *msg, bool shutdown);
void shutdownRadio(void);
uint32_t getFrequency(freq_conf_t *config);
void lockRadio(void);
void lockRadioByCamera(void);
void unlockRadio(void);

THD_FUNCTION(moduleRADIO, arg);

#endif

