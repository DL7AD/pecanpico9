#include "ch.h"
#include "hal.h"
#include "debug.h"

static module_conf_t *registered_threads[10];
static uint8_t threads_cnt = 0;

// Hardware Watchdog configuration
static const WDGConfig wdgcfg = {
	.pr =	STM32_IWDG_PR_256,
	.rlr =	STM32_IWDG_RL(10000)
};

void register_thread_at_wdg(module_conf_t *thread_config)
{
	registered_threads[threads_cnt++] = thread_config;
}

THD_FUNCTION(wdgThread, arg) {
	(void)arg;

	while(true)
	{
		bool healthy = true;
		for(uint8_t i=0; i<threads_cnt; i++) {
			if(registered_threads[i]->wdg_timeout < chVTGetSystemTimeX())
			{
				TRACE_ERROR("WDG  > Thread %s not healty", registered_threads[i]->name);
				healthy = false; // Threads reached timeout
			}
		}

		if(healthy)
			wdgReset(&WDGD1);	// Reset hardware watchdog at no error

		// Switch LEDs
		palTogglePad(PORT(IO_LED2), PIN(IO_LED2));		// Show I'M ALIVE
		chThdSleepMilliseconds(healthy ? 500 : 100);	// Blink faster (less delay) in case of an error
	}
}

void init_watchdog(void)
{
	// Initialize Watchdog
	TRACE_INFO("WDG  > Initialize Watchdog");
	wdgStart(&WDGD1, &wdgcfg);
	wdgReset(&WDGD1);

	TRACE_INFO("WDG  > Startup Watchdog thread");
	thread_t *th = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(256), "WDG", NORMALPRIO, wdgThread, NULL);
	if(!th) {
		// Print startup error, do not start watchdog for this thread
		TRACE_ERROR("TRAC > Could not startup thread (not enough memory available)");
	}
}

