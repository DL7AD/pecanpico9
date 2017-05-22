#ifndef __MODULES_H__
#define __MODULES_H__

#include "position.h"
#include "image.h"
#include "tracking.h"
#include "log.h"
#include "sgp4.h"
#include "config.h"
#include "types.h"

#define MODULE_POSITION(CONF)	{chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024*2), (CONF)->name, NORMALPRIO, modulePOS,   (CONF)); (CONF)->active=true; }
#define MODULE_IMAGE(CONF)		{chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(6*1024*2), (CONF)->name, NORMALPRIO, moduleIMG,   (CONF)); (CONF)->active=true; }
#define MODULE_ERROR(CONF)		{chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024*2), (CONF)->name, NORMALPRIO, moduleERROR, (CONF)); (CONF)->active=true; }
#define MODULE_LOG(CONF)		{chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024*2), (CONF)->name, NORMALPRIO, moduleLOG,   (CONF)); (CONF)->active=true; }
#define MODULE_TRACKING(CYCLE)	 chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024*2), "Tracking",   NORMALPRIO, moduleTRACKING, NULL  );

#define initEssentialModules() { \
	chMtxObjectInit(&interference_mtx); \
	chMtxObjectInit(&camera_mtx); \
	chMtxObjectInit(&radio_mtx); \
	MODULE_TRACKING(CYCLE_TIME); /* Tracker data input */ \
	chThdSleepMilliseconds(1000); /* Give Tracking manager some time to fill first track point */ \
}

extern char *SMODE_STRING[];
extern char *MOULATION_STRING[];
extern char *PROTOCOL_STRING[];
#define VAL2SMODE(v) SMODE_STRING[v]			/* Returns sleep as string */
#define VAL2MOULATION(v) MOULATION_STRING[v]	/* Returns modulation as string */
#define VAL2PROTOCOL(v) PROTOCOL_STRING[v]		/* Returns protocol as string */

extern mutex_t interference_mtx;	// HF interference mutex (needed to exclude radio from HF sensitiv components [Camera])

extern systime_t watchdog_tracking;	// Last update time for module TRACKING

#endif
