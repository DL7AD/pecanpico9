#ifndef __TRACKING_H__
#define __TRACKING_H__

#include "ch.h"
#include "hal.h"
#include "ptime.h"

typedef enum {
	GPS_LOCKED,		// GPS is locked and could aquire a fix
	GPS_LOSS,		// GPS was switched on all time but it couln't aquire a fix
	GPS_LOWBATT,	// GPS was switched on but had to be switched off prematurely while the battery is almost empty (or is too cold)
	GPS_LOG,		// The tracker has been just switched on and the position has been taken from the log
	GPS_OFF,		// There is no active position thread so the GPS was never switched on (in oder to save power)
} gpsLock_t;

typedef struct {
	uint32_t id;			// Serial ID
	ptime_t time;			// GPS time

	// GPS
	gpsLock_t gps_lock;		// 0: locked, 1: GPS loss, 2: low power (switched off), 3: taken from log, 4: GPS switch off permanently
	int32_t gps_lat;		// Latitude in °*10^7
	int32_t gps_lon;		// Longitude in °*10^7
	int32_t gps_alt;		// Altitude in meter
	uint8_t gps_sats;		// Satellites used for solution
	uint8_t gps_ttff;		// Time to first fix in seconds

	// Voltage and current measurement
	uint16_t adc_vsol;		// Current solar voltage in mV
	uint16_t adc_vbat;		// Current battery voltage in mV
	uint16_t adc_vusb;		// Current USB voltage in mV
	int16_t adc_pbat;		// Average battery current (since last track point)
	int16_t adc_rbat;		// Battery impedance

	// BME280 (on board)
	uint32_t air_press;		// Airpressure in Pa*10 (in 0.1Pa)
	uint16_t air_hum;		// Rel. humidity in %*10 (in 0.1%)
	int16_t air_temp;		// Temperature in degC*100 (in 0.01°C)

	int8_t id_image;		// Last image ID (this is important because it will set the image counter at reset so the last image wont get overwritten with the same image ID)
} trackPoint_t;

void waitForNewTrackPoint(void);
trackPoint_t* getLastTrackPoint(void);
void getNextLogTrackPoint(trackPoint_t* log);
void init_tracking_manager(bool useGPS);
trackPoint_t* getLogBuffer(uint16_t id);

#endif

