#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "threads.h"
#include "tracking.h"
#include "watchdog.h"
#include "image.h"
#include "pi2c.h"
#include "pac1720.h"

systime_t watchdog_tracking;

void start_essential_threads(void) {
	// Init watchdog
	init_watchdog();

	// Initialize essential mutex's
	chMtxObjectInit(&camera_mtx);
	chMtxObjectInit(&radio_mtx);

	pi2cInit();					// Initialize I2C
	pac1720_init();				// Initialize current measurement
}

