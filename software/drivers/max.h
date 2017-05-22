/**
  * @see https://github.com/thasti/utrak
  */

#ifndef __MAX_H__
#define __MAX_H__

#include "ch.h"
#include "hal.h"
#include "ptime.h"

#define UBLOX_MAX_ADDRESS	0x42

#define isGPSLocked(pos) ((pos)->type == 3 && (pos)->num_svs >= 5)

typedef struct {
	ptime_t time;		// Time
	uint8_t type;		// type of fix (validity)
	uint8_t num_svs;	// number of satellites used for solution, range 0 .. 19
	int32_t lat;		// latitude in deg * 10^7, range -90 .. +90 * 10^7
	int32_t lon;		// longitude in deg * 10^7, range -180 .. +180 * 10^7
	int32_t alt;		// altitude in m, range 0m, up to ~40000m, clamped
} gpsFix_t;

uint8_t gps_set_gps_only(void);
uint8_t gps_disable_nmea_output(void);
uint8_t gps_set_airborne_model(void);
uint8_t gps_set_power_save(void);
uint8_t gps_power_save(int on);
//uint8_t gps_save_settings(void);
bool gps_get_fix(gpsFix_t *fix);

bool GPS_Init(void);
void GPS_Deinit(void);
uint32_t GPS_get_mcu_frequency(void);

#endif

