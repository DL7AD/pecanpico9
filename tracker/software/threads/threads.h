#ifndef __MODULES_H__
#define __MODULES_H__

#include "ch.h"

void start_essential_threads(void);

extern systime_t watchdog_tracking;	// Last update time for module TRACKING

#endif

