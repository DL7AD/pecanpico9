#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "ptime.h"
#include "config.h"
#include "debug.h"
#include "modules.h"
#include "padc.h"
#include "pi2c.h"
#include "pac1720.h"
#include "bme280.h"
#include "usbcfg.h"

static virtual_timer_t vt;			// Virtual timer for LED blinking
uint32_t counter = 0;				// Main thread counter
bool error = 0;						// Error LED flag
systime_t wdg_buffer = S2ST(60);	// Software thread monitor buffer, this is the time margin for
									// a thread to react after its actual window expired, after
									// expiration watchdog will not reset anymore which will reset
									// the complete MCU

// Hardware Watchdog configuration
static const WDGConfig wdgcfg = {
	.pr =	STM32_IWDG_PR_256,
	.rlr =	STM32_IWDG_RL(10000)
};

/**
  * LED blinking routine
  * RED LED blinks: One or more modules crashed (software watchdog) INFO: Due to hardware bug, the LED cannot be used (pin = OSC_OUT => must be left floating)
  * GREEN LED blinks: I'm alive! (STM32 crashed if not blinking)
  * YELLOW LED: Camera takes a photo (See image.c)
  */
static void led_cb(void *led_sw) {
	// Switch LEDs
	palWritePad(PORT(IO_LED3), PIN(IO_LED3), (bool)led_sw);	// Show I'M ALIVE
	if(error) {
		palWritePad(PORT(IO_LED1), PIN(IO_LED1), (bool)led_sw);	// Show error
	} else {
		palSetPad(PORT(IO_LED1), PIN(IO_LED1));	// Shut off error
	}

	led_sw = (void*)!led_sw; // Set next state

	chSysLockFromISR();
	chVTSetI(&vt, MS2ST(500), led_cb, led_sw);
	chSysUnlockFromISR();
}

/**
  * Main routine is starting up system, runs the software watchdog (module monitoring), controls LEDs
  */
int main(void) {
	// Switch on oscillator
	/*RCC->AHB1ENR |= 0x18;
	PORT(OSC_P1)->MODER  = PIN_MODE_OUTPUT(PIN(OSC_P1));
	PORT(OSC_P2)->MODER |= PIN_MODE_OUTPUT(PIN(OSC_P2));
	PORT(OSC_P3)->MODER |= PIN_MODE_OUTPUT(PIN(OSC_P3));
	palSetPad(PORT(OSC_P1), PIN(OSC_P1));
	palSetPad(PORT(OSC_P2), PIN(OSC_P2));
	palSetPad(PORT(OSC_P3), PIN(OSC_P3));

	for(uint32_t i=0; i<20000000; i++);*/

	halInit();					// Startup HAL
	chSysInit();				// Startup RTOS

	boost_voltage(true);

	/*sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);*/

	/*
	* Activates the USB driver and then the USB bus pull-up on D+.
	* Note, a delay is inserted in order to not have to disconnect the cable
	* after a reset.
	*/
	/*usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(3000);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);*/


	DEBUG_INIT();				// Debug Init (Serial debug port, LEDs)
	TRACE_INFO("MAIN > Startup");

	// Initialize Watchdog
	TRACE_INFO("MAIN > Initialize Watchdog");
	wdgStart(&WDGD1, &wdgcfg);
	wdgReset(&WDGD1);

	pi2cInit();					// Startup I2C
	initEssentialModules();		// Startup required modules (input/output modules)
	initModules();				// Startup optional modules (eg. POSITION, LOG, ...)
	pac1720_init();				// Startup current measurement

	chThdSleepMilliseconds(100);

	// Initialize LED timer
	chVTObjectInit(&vt);
	chVTSet(&vt, MS2ST(500), led_cb, 0);

	chThdSleepMilliseconds(1000);

	while(true) {
		// Print time every 10 sec
		if(counter % 10 == 0)
			PRINT_TIME("MAIN");

		// Thread monitor
		bool aerror = false; // Temporary error flag
		bool healthy;
		systime_t lu;

		for(uint8_t i=0; i<sizeof(config)/sizeof(module_conf_t); i++) {
			
			if(config[i].active) { // Is active?

				// Determine health
				healthy = true;
				switch(config[i].trigger.type)
				{
					case TRIG_ONCE:
						healthy = true;
						break;

					case TRIG_EVENT:
						switch(config[i].trigger.event)
						{
							case NO_EVENT:
								healthy = true;
								break;
							case EVENT_NEW_POINT:
								healthy = config[i].last_update + S2ST(TRACK_CYCLE_TIME*2) + wdg_buffer > chVTGetSystemTimeX();
								break;
						}
						break;

					case TRIG_TIMEOUT:
						healthy = config[i].last_update + S2ST(config[i].trigger.timeout) + wdg_buffer > chVTGetSystemTimeX();
						break;

					case TRIG_CONTINOUSLY:
						healthy = config[i].last_update + wdg_buffer > chVTGetSystemTimeX();
						break;
				}
				healthy = healthy || config[i].init_delay + wdg_buffer > chVTGetSystemTimeX();

				// Debugging every 10 sec
				if(counter % 10 == 0) {
					lu = chVTGetSystemTimeX() - config[i].last_update;
					if(healthy) {
						TRACE_INFO("WDG  > Module %s OK (last activity %d.%03d sec ago)", config[i].name, ST2MS(lu)/1000, ST2MS(lu)%1000);
					} else {
						TRACE_ERROR("WDG  > Module %s failed (last activity %d.%03d sec ago)", config[i].name, ST2MS(lu)/1000, ST2MS(lu)%1000);
					}
				}

				if(!healthy)
					aerror = true; // Set error flag

			}
		}

		// Watchdog TRACKING
		healthy = watchdog_tracking + S2ST(TRACK_CYCLE_TIME) + wdg_buffer > chVTGetSystemTimeX();
		lu = chVTGetSystemTimeX() - watchdog_tracking;
		if(counter % 10 == 0) {
			if(healthy) {
				TRACE_INFO("WDG  > Module TRAC OK (last activity %d.%03d sec ago)", ST2MS(lu)/1000, ST2MS(lu)%1000);
			} else {
				TRACE_ERROR("WDG  > Module TRAC failed (last activity %d.%03d sec ago)", ST2MS(lu)/1000, ST2MS(lu)%1000);
			}
		}
		if(!healthy)
			aerror = true; // Set error flag

		// Update hardware (LED, WDG)
		error = aerror;			// Update error LED flag
		if(!error)
		{
			wdgReset(&WDGD1);	// Reset hardware watchdog at no error
		} else {
			TRACE_ERROR("WDG  > No reset");
		}

		chThdSleepMilliseconds(1000);
		counter++;
	}
}

