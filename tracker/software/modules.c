#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "modules.h"
#include "tracking.h"
#include "watchdog.h"
#include "pi2c.h"
#include "pac1720.h"

mutex_t interference_mtx;
systime_t watchdog_tracking;

void start_essential_threads(void) {
	// Init watchdog
	init_watchdog();

	// Initialize essential mutex's
	chMtxObjectInit(&interference_mtx);
	chMtxObjectInit(&camera_mtx);
	chMtxObjectInit(&radio_mtx);

	pi2cInit();					// Initialize I2C
	pac1720_init();				// Initialize current measurement

	// Start tracking manager
	init_tracking_manager();
	chThdSleepMilliseconds(1000);	// Give Tracking manager some time to fill first track point
}

