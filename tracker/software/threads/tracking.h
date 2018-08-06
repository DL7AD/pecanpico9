#ifndef __TRACKING_H__
#define __TRACKING_H__

#include "ch.h"
#include "hal.h"
#include "ptime.h"


#define BME_STATUS_BITS         2
#define BME_STATUS_MASK         0x3
#define BME_OK_VALUE            0x0
#define BME_FAIL_VALUE          0x1
#define BME_NOT_FITTED_VALUE    0x2

#define BME_ALL_STATUS_MASK     0x3F
#define BME_ALL_STATUS_SHIFT    8

#define BMEI1_STATUS_SHIFT      BME_ALL_STATUS_SHIFT
#define BMEI1_STATUS_MASK       (BME_STATUS_MASK << BMEI1_STATUS_SHIFT)

#define BMEE1_STATUS_SHIFT      BMEI1_STATUS_SHIFT + BME_STATUS_BITS
#define BMEE1_STATUS_MASK       (BME_STATUS_MASK << BMEE1_STATUS_SHIFT)

#define BMEE2_STATUS_SHIFT      BMEI1_STATUS_SHIFT + BME_STATUS)BITS
#define BMEE2_STATUS_MASK       (BME_STATUS_MASK << BMEE2_STATUS_SHIFT)


typedef enum {
	GPS_LOCKED1,	// The GPS is locked, the GPS has been switched off
	GPS_LOCKED2,	// The GPS is locked, the GPS has been kept switched on
	GPS_LOSS,		// The GPS was switched on all time but it couln't aquire a fix
	GPS_LOWBATT1,	// The GPS wasnt switched on because the battery has not enough energy
	GPS_LOWBATT2,	// The GPS was switched on but has been switched off prematurely while the battery has not enough energy (or is too cold)
	GPS_LOG,		// The tracker has been just switched on and the position has been taken from the log
	GPS_OFF,		// There is no active position thread so the GPS was never switched on (in oder to save power)
	GPS_ERROR		// The GPS has a communication error
} gpsLock_t;

typedef struct {
	// Voltage and current measurement
	uint16_t adc_vsol;		// Current solar voltage in mV
	uint16_t adc_vbat;		// Current battery voltage in mV
	uint16_t pac_vsol;
	uint16_t pac_vbat;
	int16_t pac_pbat;
	int16_t pac_psol;

	uint16_t light_intensity;

	// GPS
	gpsLock_t gps_lock;		// 0: locked, 1: GPS loss, 2: low power (switched off), 3: taken from log, 4: GPS switch off permanently
	uint8_t gps_sats;		// Satellites used for solution
	uint8_t gps_ttff;		// Time to first fix in seconds
	uint8_t gps_pdop;		// Position DOP in 0.05 per unit (unitless)
	uint16_t gps_alt;		// Altitude in meter
	int32_t gps_lat;		// Latitude in 10^(-7)° per unit
	int32_t gps_lon;		// Longitude in 10^(-7)° per unit

	// BME280 (on board)
	uint32_t sen_i1_press;		// Airpressure in Pa*10 (in 0.1Pa)
	uint32_t sen_e1_press;		// Airpressure in Pa*10 (in 0.1Pa)
	uint32_t sen_e2_press;		// Airpressure in Pa*10 (in 0.1Pa)

	int16_t sen_i1_temp;		// Temperature in 0.01°C per unit
	int16_t sen_e1_temp;		// Temperature in 0.01°C per unit
	int16_t sen_e2_temp;		// Temperature in 0.01°C per unit

	uint8_t sen_i1_hum;			// Rel. humidity in %
	uint8_t sen_e1_hum;			// Rel. humidity in %
	uint8_t sen_e2_hum;			// Rel. humidity in %

	uint8_t dummy2;

	int16_t stm32_temp;
	int16_t si4464_temp;

	uint16_t reset;
	uint32_t id;			// Serial ID
	uint32_t gps_time;		// GPS time

	uint32_t sys_time;		// System time (in seconds)
	uint32_t sys_error;			// System error flags
                      /*
                       * Set system errors.
                       *
                       * Bit usage:
                       * -  0:1  I2C status
                       * -  2:2  GPS status
                       * -  3:4  pac1720 status
                       * -  5:7  OV5640 status
                       * -  8:9  BMEi1 status (0 = OK, 1 = Fail, 2 = Not fitted)
                       * - 10:11 BMEe1 status (0 = OK, 1 = Fail, 2 = Not fitted)
                       * - 12:13 BMEe2 status (0 = OK, 1 = Fail, 2 = Not fitted)
                       */
} trackPoint_t;

void waitForNewTrackPoint(void);
trackPoint_t* getLastTrackPoint(void);
void getNextLogTrackPoint(trackPoint_t* log);
void init_tracking_manager(bool useGPS);
trackPoint_t* getLogBuffer(uint16_t id);

#endif

