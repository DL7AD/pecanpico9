#ifndef __RADIO_H__
#define __RADIO_H__

#include "ch.h"
#include "hal.h"
#include "config.h"
#include "si4464.h"
#include "modules.h"

// Preprocessor macros for config file
#define APRS_REGION_FREQ_2M			getAPRSRegionFrequency2m
#define APRS_REGION_FREQ_70CM		getAPRSRegionFrequency70cm
#define APRS_ISS_FREQ				getAPRSISSFrequency

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

extern mutex_t radio_mtx;

uint32_t getAPRSRegionFrequency2m(void);
uint32_t getAPRSRegionFrequency70cm(void);
uint32_t getAPRSISSFrequency(void);
bool transmitOnRadio(radioMSG_t *msg);
uint32_t getFrequency(freuquency_config_t *config);

THD_FUNCTION(moduleRADIO, arg);

#endif

