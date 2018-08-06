#include "ch.h"
#include "hal.h"

#include "tracking.h"
#include "debug.h"
#include "config.h"
#include "ublox.h"
#include "bme280.h"
#include "padc.h"
#include "pac1720.h"
#include "ov5640.h"
#include "radio.h"
#include "flash.h"
#include "watchdog.h"
#include "pi2c.h"

static trackPoint_t trackPoints[2];
static trackPoint_t* lastTrackPoint;
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


static void aquirePosition(trackPoint_t* tp, trackPoint_t* ltp, systime_t timeout)
{
	systime_t start = chVTGetSystemTimeX();

	gpsFix_t gpsFix;
	memset(&gpsFix, 0, sizeof(gpsFix_t));

	// Switch on GPS if enough power is available and GPS is needed by any position thread
	uint16_t batt = stm32_get_vbat();
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
				batt = stm32_get_vbat();
				gps_get_fix(&gpsFix);
			} while(!isGPSLocked(&gpsFix) && batt >= gps_off_vbat && chVTGetSystemTimeX() <= start + timeout); // Do as long no GPS lock and within timeout, timeout=cycle-1sec (-3sec in order to keep synchronization)

			if(batt < gps_off_vbat) { // GPS was switched on but prematurely switched off because the battery is low on power, switch off GPS

				GPS_Deinit();
				TRACE_WARN("TRAC > GPS sampling finished GPS LOW BATT");
				tp->gps_lock = GPS_LOWBATT2;

			} else if(!isGPSLocked(&gpsFix)) { // GPS was switched on but it failed to get a lock, keep GPS switched on

				TRACE_WARN("TRAC > GPS sampling finished GPS LOSS");
				tp->gps_lock = GPS_LOSS;

			} else { // GPS locked successfully, switch off GPS (unless cycle is less than 60 seconds)

				// Switch off GPS (if cycle time is more than 60 seconds)
				if(track_cycle_time < S2ST(60)) {
					TRACE_INFO("TRAC > Keep GPS switched on because cycle < 60sec");
					tp->gps_lock = GPS_LOCKED2;
				} else if(gps_onper_vbat != 0 && batt >= gps_onper_vbat) {
					TRACE_INFO("TRAC > Keep GPS switched on because VBAT >= %dmV", gps_onper_vbat);
					tp->gps_lock = GPS_LOCKED2;
				} else {
					TRACE_INFO("TRAC > Switch off GPS");
					GPS_Deinit();
					tp->gps_lock = GPS_LOCKED1;
				}

				// Debug
				TRACE_INFO("TRAC > GPS sampling finished GPS LOCK");

				// Calibrate RTC
				setTime(&gpsFix.time);

				// Take time from GPS
				tp->gps_time = date2UnixTimestamp(&gpsFix.time);

				// Set new GPS fix
				tp->gps_lat = gpsFix.lat;
				tp->gps_lon = gpsFix.lon;
				tp->gps_alt = gpsFix.alt;

				tp->gps_sats = gpsFix.num_svs;
				tp->gps_pdop = (gpsFix.pdop+3)/5;
			}

		} else { // GPS communication error

			//GPS_Deinit();
			tp->gps_lock = GPS_ERROR;

		}
	}

	tp->gps_ttff = ST2S(chVTGetSystemTimeX() - start); // Time to first fix

	if(tp->gps_lock != GPS_LOCKED1 && tp->gps_lock != GPS_LOCKED2) { // We have no valid GPS fix
		// Take time from internal RTC
		ptime_t time;
		getTime(&time);
		tp->gps_time = date2UnixTimestamp(&time);

		// Take GPS fix from old lock
		tp->gps_lat = ltp->gps_lat;
		tp->gps_lon = ltp->gps_lon;
		tp->gps_alt = ltp->gps_alt;
	}
}

static void measureVoltage(trackPoint_t* tp)
{
	tp->adc_vbat = stm32_get_vbat();
	tp->adc_vsol = stm32_get_vsol();

	pac1720_get_avg(&tp->pac_vbat, &tp->pac_vsol, &tp->pac_pbat, &tp->pac_psol);
	tp->pac_pbat = -tp->pac_pbat;
}

static uint8_t bme280_error;

static void getSensors(trackPoint_t* tp)
{
	// Measure BME280
	bme280_t handle;

	if(BME280_isAvailable(BME280_ADDRESS_INT)) {
		BME280_Init(&handle, BME280_ADDRESS_INT);
		tp->sen_i1_press = BME280_getPressure(&handle, 256);
		tp->sen_i1_hum = BME280_getHumidity(&handle);
		tp->sen_i1_temp = BME280_getTemperature(&handle);
		bme280_error = 0x0;
	} else { // No internal BME280 found
		TRACE_ERROR("TRAC > Internal BME280 not available");
		tp->sen_i1_press = 0;
		tp->sen_i1_hum = 0;
		tp->sen_i1_temp = 0;
		bme280_error = 0x1;
	}
	bme280_error |= 0x0A;

	// Measure various temperature sensors
	tp->stm32_temp = stm32_get_temp();
	tp->si4464_temp = Si4464_getLastTemperature();

	// Measure light intensity from OV5640
	tp->light_intensity = OV5640_getLastLightIntensity() & 0xFFFF;
}

static void setSystemStatus(trackPoint_t* tp) {

  /*
   * Set system errors.
   *
   * Bit usage:
   * -  0:1  I2C status
   * -  2:2  GPS status
   * -  3:4  pac1720 status
   * -  5:7  OV5640 status
   * -  8:9  BMEi1 status (0 = OK, 1 = Fail, 2 = Not fitted)
   * -  10:11 BMEe1 status (0 = OK, 1 = Fail, 2 = Not fitted)
   * -  12:13 BMEe2 status (0 = OK, 1 = Fail, 2 = Not fitted)
   */
	tp->sys_error = 0;

	tp->sys_error |= (I2C_hasError()     & 0x1)   << 0;
	tp->sys_error |= (tp->gps_lock == GPS_ERROR)  << 2;
	tp->sys_error |= (pac1720_hasError() & 0x3)   << 3;
	tp->sys_error |= (OV5640_hasError()  & 0x7)   << 5;

	tp->sys_error |= (bme280_error & BME_ALL_STATUS_MASK)
	    << BME_ALL_STATUS_SHIFT;

	// Set system time
	tp->sys_time = ST2S(chVTGetSystemTimeX());
}

/**
  * Tracking Module (Thread)
  */
THD_FUNCTION(trackingThread, arg) {
	(void)arg;

	uint32_t id = 0;
	lastTrackPoint = &trackPoints[0];

	// Read time from RTC
	ptime_t time;
	getTime(&time);
	lastTrackPoint->gps_time = date2UnixTimestamp(&time);

	// Get last tracking point from memory
	TRACE_INFO("TRAC > Read last track point from flash memory");
	trackPoint_t* lastLogPoint = getLastLog();

	if(lastLogPoint != NULL) { // If there has been stored a trackpoint, then get the last know GPS fix
		trackPoints[0].reset     = lastLogPoint->reset+1;
		trackPoints[1].reset     = lastLogPoint->reset+1;
		lastTrackPoint->gps_lat  = lastLogPoint->gps_lat;
		lastTrackPoint->gps_lon  = lastLogPoint->gps_lon;
		lastTrackPoint->gps_alt  = lastLogPoint->gps_alt;
		lastTrackPoint->gps_sats = lastLogPoint->gps_sats;
		lastTrackPoint->gps_ttff = lastLogPoint->gps_ttff;

		TRACE_INFO(
			"TRAC > Last track point (from memory)\r\n"
			"%s Reset %d ID %d\r\n"
			"%s Latitude: %d.%07ddeg\r\n"
			"%s Longitude: %d.%07ddeg\r\n"
			"%s Altitude: %d Meter",
			TRACE_TAB, lastLogPoint->reset, lastLogPoint->id,
			TRACE_TAB, lastTrackPoint->gps_lat/10000000, (lastTrackPoint->gps_lat > 0 ? 1:-1)*lastTrackPoint->gps_lat%10000000,
			TRACE_TAB, lastTrackPoint->gps_lon/10000000, (lastTrackPoint->gps_lon > 0 ? 1:-1)*lastTrackPoint->gps_lon%10000000,
			TRACE_TAB, lastTrackPoint->gps_alt
		);
	} else {
		TRACE_INFO("TRAC > No track point found in flash memory");
	}

	lastTrackPoint->gps_lock = GPS_LOG; // Mark trackPoint as LOG packet

	// Initialize Si4464 to get Temperature readout
	Si4464_Init();
	Si4464_shutdown();

	// Measure telemetry
	measureVoltage(lastTrackPoint);
	getSensors(lastTrackPoint);
	setSystemStatus(lastTrackPoint);

	// Write Trackpoint to Flash memory
	writeLogTrackPoint(lastTrackPoint);

	// Wait for position threads to start
	chThdSleepMilliseconds(100);

	systime_t cycle_time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("TRAC > Do module TRACKING MANAGER cycle");
		trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		trackPoint_t* tp  = &trackPoints[(id+1) % 2]; // Current track point (the one which is processed now)
		trackPoint_t* ltp = &trackPoints[ id    % 2]; // Last track point

		// Get GPS position
		aquirePosition(tp, ltp, track_cycle_time - S2ST(3));

		tp->id = ++id; // Serial ID

		// Measure telemetry
		measureVoltage(tp);
		getSensors(tp);
		setSystemStatus(tp);

		// Trace data
		unixTimestamp2Date(&time, tp->gps_time);
		TRACE_INFO(	"TRAC > New tracking point available (ID=%d)\r\n"
					"%s Time %04d-%02d-%02d %02d:%02d:%02d\r\n"
					"%s Pos  %d.%05d %d.%05d Alt %dm\r\n"
					"%s Sats %d TTFF %dsec\r\n"
					"%s ADC Vbat=%d.%03dV Vsol=%d.%03dV Pbat=%dmW\r\n"
					"%s AIR p=%6d.%01dPa T=%2d.%02ddegC phi=%2d.%01d%%",
					tp->id,
					TRACE_TAB, time.year, time.month, time.day, time.hour, time.minute, time.day,
					TRACE_TAB, tp->gps_lat/10000000, (tp->gps_lat > 0 ? 1:-1)*(tp->gps_lat/100)%100000, tp->gps_lon/10000000, (tp->gps_lon > 0 ? 1:-1)*(tp->gps_lon/100)%100000, tp->gps_alt,
					TRACE_TAB, tp->gps_sats, tp->gps_ttff,
					TRACE_TAB, tp->adc_vbat/1000, (tp->adc_vbat%1000), tp->adc_vsol/1000, (tp->adc_vsol%1000), tp->pac_pbat,
					TRACE_TAB, tp->sen_i1_press/10, tp->sen_i1_press%10, tp->sen_i1_temp/100, tp->sen_i1_temp%100, tp->sen_i1_hum/10, tp->sen_i1_hum%10
		);

		// Write Trackpoint to Flash memory
		writeLogTrackPoint(tp);

		// Switch last track point
		lastTrackPoint = tp;

		// Wait until cycle
		cycle_time = chThdSleepUntilWindowed(cycle_time, cycle_time + track_cycle_time);
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
		thread_t *th = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024), "TRA", NORMALPRIO+1, trackingThread, NULL);
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

