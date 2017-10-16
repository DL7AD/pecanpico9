#include "ch.h"
#include "hal.h"

#include "tracking.h"
#include "debug.h"
#include "config.h"
#include "ublox.h"
#include "bme280.h"
#include "padc.h"
#include "pac1720.h"
#include "radio.h"
#include "flash.h"
#include "watchdog.h"
#include "image.h"

static trackPoint_t trackPoints[2];
static trackPoint_t* lastTrackPoint;
static systime_t nextLogEntryTimer;
static module_conf_t trac_conf = {.name = "TRAC"}; // Fake config needed for watchdog tracking
static bool threadStarted = false;
static bool tracking_useGPS = false;

/**
  * Returns most recent track point witch is complete.
  */
trackPoint_t* getLastTrackPoint(void)
{
	return lastTrackPoint;
}

trackPoint_t* getLogBuffer(uint16_t id)
{
	if(sizeof(trackPoint_t)*id < LOG_SECTOR_SIZE-sizeof(trackPoint_t))
	{
		return (trackPoint_t*)(LOG_FLASH_ADDR1 + id * sizeof(trackPoint_t));
	} else if((id-(LOG_SECTOR_SIZE/sizeof(trackPoint_t)))*sizeof(trackPoint_t) < LOG_SECTOR_SIZE-sizeof(trackPoint_t)) {
		return (trackPoint_t*)(LOG_FLASH_ADDR2 + (id-(LOG_SECTOR_SIZE/sizeof(trackPoint_t))) * sizeof(trackPoint_t));
	} else { // Outside of memory address allocation
		return NULL;
	}
}

/**
  * Returns next free log entry address in memory. Returns 0 if all cells are
  * filled with data
  */
static trackPoint_t* getNextFreeLogAddress(void)
{
	trackPoint_t* tp;
	for(uint16_t i=0; (tp = getLogBuffer(i)) != NULL; i++)
		if(tp->id == 0xFFFFFFFF)
			return tp;

	return NULL;
}

/**
  * Returns next free log entry address in memory. Returns 0 if all cells are
  * filled with data
  */
static trackPoint_t* getLastLog(void)
{
	trackPoint_t* last = NULL;
	trackPoint_t* tp;
	for(uint16_t i=0; (tp = getLogBuffer(i)) != NULL; i++) {
		if(tp->id == 0xFFFFFFFF)
			return last; // Found last entry
		last = tp;
	}
	if(last->id != 0xFFFFFFFF)
		return last; // All memory entries are use, so the very last one must be the most recent one.
	return NULL; // There is no log entry in memory
}

/**
  * Erases oldest data
  */
static void eraseOldestLogData(void)
{
	// Determine which sector holds the oldest data
	trackPoint_t pt1, pt2;
	flashRead(LOG_FLASH_ADDR1, (char*)&pt1, sizeof(trackPoint_t));
	flashRead(LOG_FLASH_ADDR2, (char*)&pt2, sizeof(trackPoint_t));

	if(pt1.id < pt2.id) // Erase sector 10
	{
		TRACE_INFO("TRAC > Erase flash %08x", LOG_FLASH_ADDR1);
		flashErase(LOG_FLASH_ADDR1, LOG_SECTOR_SIZE);
	} else { // Erase sector 11
		TRACE_INFO("TRAC > Erase flash %08x", LOG_FLASH_ADDR2);
		flashErase(LOG_FLASH_ADDR2, LOG_SECTOR_SIZE);
	}
}

static void writeLogTrackPoint(trackPoint_t* tp)
{
	// Get address to write on
	trackPoint_t* address = getNextFreeLogAddress();
	if(address == NULL) // Memory completly used, erase oldest data
	{
		eraseOldestLogData();
		address = getNextFreeLogAddress();
	}
	if(address == NULL) // Something went wront at erasing the memory
	{
		TRACE_ERROR("TRAC > Erasing flash failed");
		return;
	}

	// Write data into flash
	TRACE_INFO("TRAC > Flash write (ADDR=%08x)", address);
	flashSectorBegin(flashSectorAt((uint32_t)address));
	flashWrite((uint32_t)address, (char*)tp, sizeof(trackPoint_t));
	flashSectorEnd(flashSectorAt((uint32_t)address));

	// Verify
	if(flashCompare((uint32_t)address, (char*)tp, sizeof(trackPoint_t)))
		TRACE_INFO("TRAC > Flash write OK")
	else
		TRACE_ERROR("TRAC > Flash write failed");
}

void waitForNewTrackPoint(void)
{
	uint32_t old_id = getLastTrackPoint()->id;
	while(old_id == getLastTrackPoint()->id)
		chThdSleepMilliseconds(1000);
}

/**
  * Tracking Module (Thread)
  */
THD_FUNCTION(trackingThread, arg) {
	(void)arg;

	uint32_t id = 1;
	lastTrackPoint = &trackPoints[0];

	// Fill initial values by PAC1720 and BME280 and RTC

	// Time
	ptime_t rtc;
	getTime(&rtc);
	lastTrackPoint->time.year = rtc.year;
	lastTrackPoint->time.month = rtc.month;
	lastTrackPoint->time.day = rtc.day;
	lastTrackPoint->time.hour = rtc.hour;
	lastTrackPoint->time.minute = rtc.minute;
	lastTrackPoint->time.second = rtc.second;

	// Get last tracking point from memory
	TRACE_INFO("TRAC > Read last track point from flash memory");
	trackPoint_t* lastLogPoint = getLastLog();

	if(lastLogPoint != NULL) { // If there has been stored a trackpoint, then get the last know GPS fix
		TRACE_INFO("TRAC > Found track point in flash memory ID=%d", lastLogPoint->id);
		id = lastLogPoint->id+1;
		lastTrackPoint->gps_lat = lastLogPoint->gps_lat;
		lastTrackPoint->gps_lon = lastLogPoint->gps_lon;
		lastTrackPoint->gps_alt = lastLogPoint->gps_alt;
	} else {
		TRACE_INFO("TRAC > No track point found in flash memory");
	}

	lastTrackPoint->gps_lock = GPS_LOG; // Tell other threads that it has been taken from log
	lastTrackPoint->gps_sats = 0;
	lastTrackPoint->gps_ttff = 0;

	// Debug last stored GPS position
	if(lastLogPoint != NULL) {
		TRACE_INFO(
			"TRAC > Last GPS position (from memory)\r\n"
			"%s Latitude: %d.%07ddeg\r\n"
			"%s Longitude: %d.%07ddeg\r\n"
			"%s Altitude: %d Meter",
			TRACE_TAB, lastTrackPoint->gps_lat/10000000, (lastTrackPoint->gps_lat > 0 ? 1:-1)*lastTrackPoint->gps_lat%10000000,
			TRACE_TAB, lastTrackPoint->gps_lon/10000000, (lastTrackPoint->gps_lon > 0 ? 1:-1)*lastTrackPoint->gps_lon%10000000,
			TRACE_TAB, lastTrackPoint->gps_alt
		);
	} else {
		TRACE_INFO("TRAC > No GPS position in memory");
	}

	// Voltage/Current
	lastTrackPoint->adc_vsol = getSolarVoltageMV();
	lastTrackPoint->adc_vbat = getBatteryVoltageMV();
	lastTrackPoint->adc_vusb = getUSBVoltageMV();
	lastTrackPoint->adc_pbat = pac1720_getPbat();

	bme280_t bme280;

	// Atmosphere condition
	if(BME280_isAvailable(BME280_ADDRESS_INT)) {
		BME280_Init(&bme280, BME280_ADDRESS_INT);
		lastTrackPoint->air_press = BME280_getPressure(&bme280, 256);
		lastTrackPoint->air_hum = BME280_getHumidity(&bme280);
		lastTrackPoint->air_temp = BME280_getTemperature(&bme280);
	} else { // No internal BME280 found
		TRACE_ERROR("TRAC > No BME280 found");
		lastTrackPoint->air_press = 0;
		lastTrackPoint->air_hum = 0;
		lastTrackPoint->air_temp = 0;
	}

	/*
	 * Get last image ID. This is important because Habhub does mix up different
	 * images with the same it. So it is good to use a new image ID when the
	 * tracker has been reset.
	 */
	if(lastLogPoint != NULL)
		gimage_id = lastLogPoint->id_image+1;

	systime_t time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("TRAC > Do module TRACKING MANAGER cycle");
		trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		trackPoint_t* tp = &trackPoints[id % (sizeof(trackPoints) / sizeof(trackPoint_t))]; // Current track point
		trackPoint_t* ltp = &trackPoints[(id-1) % (sizeof(trackPoints) / sizeof(trackPoint_t))]; // Last track point

		// Search for GPS satellites
		gpsFix_t gpsFix = {{0,0,0,0,0,0,0},0,0,0,0,0};

		// Switch on GPS is enough power is available and GPS is needed by any position thread
		uint16_t batt = getBatteryVoltageMV();
		if(!tracking_useGPS) { // No position thread running
			tp->gps_lock = GPS_OFF;
		} else if(batt < gps_on_vbat) {
			tp->gps_lock = GPS_LOWBATT1;
		} else {

			// Switch on GPS
			bool status = GPS_Init();

			if(status) {

				// Search for lock as long enough power is available
				do {
					batt = getBatteryVoltageMV();
					gps_get_fix(&gpsFix);
				} while(!isGPSLocked(&gpsFix) && batt >= gps_off_vbat && chVTGetSystemTimeX() <= time + track_cycle_time - S2ST(3)); // Do as long no GPS lock and within timeout, timeout=cycle-1sec (-3sec in order to keep synchronization)

				if(batt < gps_off_vbat) { // GPS was switched on but prematurely switched off because the battery is low on power, switch off GPS

					GPS_Deinit();
					TRACE_WARN("TRAC > GPS sampling finished GPS LOW BATT");
					tp->gps_lock = GPS_LOWBATT2;

				} else if(!isGPSLocked(&gpsFix)) { // GPS was switched on but it failed to get a lock, keep GPS switched on

					TRACE_WARN("TRAC > GPS sampling finished GPS LOSS");
					tp->gps_lock = GPS_LOSS;

				} else { // GPS locked successfully, switch off GPS (unless cycle is less than 60 seconds)

					// Switch off GPS (if cycle time is more than 60 seconds)
					if(track_cycle_time >= S2ST(60)) {
						TRACE_INFO("TRAC > Switch off GPS");
						GPS_Deinit();
					} else {
						TRACE_INFO("TRAC > Keep GPS switched of because cycle < 60sec");
					}

					// Debug
					TRACE_INFO("TRAC > GPS sampling finished GPS LOCK");

					// Calibrate RTC
					setTime(gpsFix.time);

					// Take time from GPS
					tp->time.year = gpsFix.time.year;
					tp->time.month = gpsFix.time.month;
					tp->time.day = gpsFix.time.day;
					tp->time.hour = gpsFix.time.hour;
					tp->time.minute = gpsFix.time.minute;
					tp->time.second = gpsFix.time.second;

					// Set new GPS fix
					tp->gps_lat = gpsFix.lat;
					tp->gps_lon = gpsFix.lon;
					tp->gps_alt = gpsFix.alt;

					tp->gps_lock = GPS_LOCKED;
					tp->gps_sats = gpsFix.num_svs;
				}

			} else { // GPS communication error

				GPS_Deinit();
				tp->gps_lock = GPS_ERROR;

			}
		}

		if(tp->gps_lock != GPS_LOCKED) { // We have no valid GPS fix
			// Take time from internal RTC
			getTime(&rtc);
			tp->time.year = rtc.year;
			tp->time.month = rtc.month;
			tp->time.day = rtc.day;
			tp->time.hour = rtc.hour;
			tp->time.minute = rtc.minute;
			tp->time.second = rtc.second;

			// Take GPS fix from old lock
			tp->gps_lat = ltp->gps_lat;
			tp->gps_lon = ltp->gps_lon;
			tp->gps_alt = ltp->gps_alt;
		}

		tp->id = id; // Serial ID
		tp->gps_ttff = ST2S(chVTGetSystemTimeX() - time); // Time to first fix

		// Power management
		tp->adc_vsol = getSolarVoltageMV();
		tp->adc_vbat = getBatteryVoltageMV();
		tp->adc_vusb = getUSBVoltageMV();
		tp->adc_pbat = pac1720_getAvgPbat();
		tp->adc_rbat = pac1720_getAvgRbat();

		bme280_t bme280;

		// Atmosphere condition
		if(BME280_isAvailable(BME280_ADDRESS_INT)) {
			BME280_Init(&bme280, BME280_ADDRESS_INT);
			tp->air_press = BME280_getPressure(&bme280, 256);
			tp->air_hum = BME280_getHumidity(&bme280);
			tp->air_temp = BME280_getTemperature(&bme280);
		} else { // No internal BME280 found
			TRACE_ERROR("TRAC > Internal BME280 not available");
			tp->air_press = 0;
			tp->air_hum = 0;
			tp->air_temp = 0;
		}

		// Set last time ID
		tp->id_image = gimage_id;

		// Trace data
		TRACE_INFO(	"TRAC > New tracking point available (ID=%d)\r\n"
					"%s Time %04d-%02d-%02d %02d:%02d:%02d\r\n"
					"%s Pos  %d.%05d %d.%05d Alt %dm\r\n"
					"%s Sats %d  TTFF %dsec\r\n"
					"%s ADC Vbat=%d.%03dV Vsol=%d.%03dV VUSB=%d.%03dV Pbat=%dmW Rbat=%dmOhm\r\n"
					"%s AIR p=%6d.%01dPa T=%2d.%02ddegC phi=%2d.%01d%% ImageID=%d",
					tp->id,
					TRACE_TAB, tp->time.year, tp->time.month, tp->time.day, tp->time.hour, tp->time.minute, tp->time.day,
					TRACE_TAB, tp->gps_lat/10000000, (tp->gps_lat > 0 ? 1:-1)*(tp->gps_lat/100)%100000, tp->gps_lon/10000000, (tp->gps_lon > 0 ? 1:-1)*(tp->gps_lon/100)%100000, tp->gps_alt,
					TRACE_TAB, tp->gps_sats, tp->gps_ttff,
					TRACE_TAB, tp->adc_vbat/1000, (tp->adc_vbat%1000), tp->adc_vsol/1000, (tp->adc_vsol%1000), tp->adc_vusb/1000, (tp->adc_vusb%1000), tp->adc_pbat, tp->adc_rbat,
					TRACE_TAB, tp->air_press/10, tp->air_press%10, tp->air_temp/100, tp->air_temp%100, tp->air_hum/10, tp->air_hum%10, tp->id_image
		);

		// Append logging (timeout)
		if(nextLogEntryTimer <= chVTGetSystemTimeX())
		{
			writeLogTrackPoint(tp);
			nextLogEntryTimer += log_cycle_time;
		}

		// Switch last recent track point
		lastTrackPoint = tp;
		id++;

		time = chThdSleepUntilWindowed(time, time + track_cycle_time); // Wait until time + cycletime
	}
}

void init_tracking_manager(bool useGPS)
{
	if(useGPS)
		tracking_useGPS = true;

	if(!threadStarted)
	{
		threadStarted = true;

		TRACE_INFO("TRAC > Startup tracking thread");
		thread_t *th = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024), "TRA", NORMALPRIO, trackingThread, NULL);
		if(!th) {
			// Print startup error, do not start watchdog for this thread
			TRACE_ERROR("TRAC > Could not startup thread (not enough memory available)");
		} else {
			register_thread_at_wdg(&trac_conf);
			trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(1);
			chThdSleepMilliseconds(300); // Wait a little bit until tracking manager has initialized first dataset
		}
	}
}

