#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "types.h"
#include "radio.h"
#include "sleep.h"

#define TRACK_CYCLE_TIME		60			/* Tracking cycle (all peripheral data [airpressure, GPS, temperature, ...] is collected each x seconds */

#define LOG_CYCLE_TIME			60			/* Log cycle time in seconds */
#define LOG_FLASH_ADDR1			0x080C0000	/* Log flash memory address 1 */
#define LOG_FLASH_ADDR2			0x080E0000	/* Log flash memory address 2 */
#define LOG_SECTOR_SIZE			0x20000		/* Log flash memory size */

#define GPS_ON_VBAT				7000			/* Battery voltage threshold at which GPS is switched on */
#define GPS_OFF_VBAT			7000			/* Battery voltage threshold at which GPS is switched off */

#define TRACE_TIME				TRUE		/* Enables time tracing on debugging port */
#define TRACE_FILE				FALSE		/* Enables file and line tracing on debugging port */

#define RUN_3V					FALSE		/* Lets the tracker run a 3V otherwise 1.8V. 3V is needed to do 20dBm radio output power.
											 * With 1.8V only 15dBm can be done. Some serial-USB adapters also need a 3V IO level in
											 * order to work. However 3V takes a lot of power in idle. You can save energy using 1.8V. */

void start_user_modules(void);
extern module_conf_t config[9];

#endif

