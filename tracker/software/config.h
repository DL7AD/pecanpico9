#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "types.h"
#include "radio.h"
#include "sleep.h"

extern module_conf_t config[9];

#define TRACK_CYCLE_TIME		60			/* Tracking cycle (all peripheral data [airpressure, GPS, temperature, ...] is collected each x seconds */

#define LOG_CYCLE_TIME			1800		/* Log cycle in seconds */

#define GPS_ON_VBAT				5000		/* Battery voltage threshold at which GPS is switched on */
#define GPS_OFF_VBAT			5000		/* Battery voltage threshold at which GPS is switched off */

#define TRACE_TIME				TRUE		/* Enables time tracing on serial connection */
#define TRACE_FILE				TRUE		/* Enables file and line tracing on serial connection */

void start_user_modules(void);

#endif

