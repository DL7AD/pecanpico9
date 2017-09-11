#include "ch.h"
#include "hal.h"

#include "threads.h"
#include "tracking.h"
#include "watchdog.h"
#include "pi2c.h"
#include "pac1720.h"

systime_t watchdog_tracking;

void start_essential_threads(void) {
	// Init watchdog
	init_watchdog();

	// Initialize tracking manager (without GPS, GPS is initialized if needed by position thread)
	init_tracking_manager(false);

	pi2cInit();					// Initialize I2C
	pac1720_init();				// Initialize current measurement
}

