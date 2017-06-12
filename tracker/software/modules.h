#ifndef __MODULES_H__
#define __MODULES_H__

#include "position.h"
#include "image.h"
#include "tracking.h"
#include "log.h"
#include "sgp4.h"
#include "config.h"
#include "types.h"

void start_essential_threads(void);

extern mutex_t interference_mtx;	// HF interference mutex (needed to exclude radio from HF sensitiv components [Camera])
extern systime_t watchdog_tracking;	// Last update time for module TRACKING

#endif

