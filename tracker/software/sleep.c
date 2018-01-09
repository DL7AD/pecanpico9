#include "ch.h"
#include "hal.h"
#include "sleep.h"
#include "padc.h"
#include "tracking.h"
#include "debug.h"
#include "padc.h"
#include "pac1720.h"

/**
  * Sleeping method. Returns true if sleeping condition are given.
  */
bool p_sleep(const sleep_conf_t *config)
{
	switch(config->type)
	{
		case SLEEP_WHEN_VBAT_BELOW_THRES:
			return stm32_get_vbat() < config->vbat_thres;

		case SLEEP_WHEN_VSOL_BELOW_THRES:
			return stm32_get_vsol() < config->vsol_thres;

		case SLEEP_WHEN_VBAT_ABOVE_THRES:
			return stm32_get_vbat() > config->vbat_thres;

		case SLEEP_WHEN_VSOL_ABOVE_THRES:
			return stm32_get_vsol() > config->vsol_thres;

		case SLEEP_WHEN_DISCHARGING:
		case SLEEP_WHEN_CHARGING:
			TRACE_WARN("Sleeping method not implemented");
			return false;

		case SLEEP_DISABLED:
			return false;
	}
	return false;
}

systime_t waitForTrigger(systime_t prev, trigger_conf_t *config)
{
	switch(config->type)
	{
		case TRIG_NEW_POINT: // Wait for new tracking point
			waitForNewTrackPoint();
			return chVTGetSystemTimeX();
		
		case TRIG_TIMEOUT: // Wait for specified timeout
			return chThdSleepUntilWindowed(prev, prev + S2ST(config->timeout));

		case TRIG_CONTINUOUSLY: // Immediate trigger
			return chVTGetSystemTimeX();

		case TRIG_ONCE: // No trigger defined
			chThdSleepMilliseconds(10000);
	}

	return chVTGetSystemTimeX();
}

void trigger_new_tracking_point(void)
{
	uint32_t oldID = getLastTrackPoint()->id;
	trackPoint_t *newtp;
	do { // Wait for new serial ID to be deployed
		chThdSleepMilliseconds(100);
		newtp = getLastTrackPoint();
	} while(newtp->id == oldID);
}

