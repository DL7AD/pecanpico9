#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "modules.h"
#include "padc.h"
#include "usbcfg.h"

/**
  * Main routine is starting up system, runs the software watchdog (module monitoring), controls LEDs
  */
int main(void) {
	halInit();					// Startup HAL
	chSysInit();				// Startup RTOS

	// Voltage switching (1.8V <=> 3.0V)
	bool usbConnected = isUsbConnected();
	if(usbConnected || RUN_3V)
	{
		boost_voltage(true); // Ramp up voltage to 3V
		chThdSleepMilliseconds(100);
	}

	// Init debugging (Serial debug port, LEDs)
	DEBUG_INIT();
	TRACE_INFO("MAIN > Startup");

	// Start USB (if connected)
	if(usbConnected)
	{
		TRACE_INFO("MAIN > USB detected");

		sduObjectInit(&SDU1);
		sduStart(&SDU1, &serusbcfg);

		usbDisconnectBus(serusbcfg.usbp);
		chThdSleepMilliseconds(100);
		usbStart(serusbcfg.usbp, &usbcfg);
		usbConnectBus(serusbcfg.usbp);
	} else {
		TRACE_INFO("MAIN > USB not detected");
	}

	// Startup threads
	start_essential_threads();	// Startup required modules (tracking managemer, watchdog)
	start_user_modules();		// Startup optional modules (eg. POSITION, LOG, ...)

	// Print time every 10 sec
	while(true) {
		PRINT_TIME("MAIN");
		chThdSleepMilliseconds(10000);
	}
}

