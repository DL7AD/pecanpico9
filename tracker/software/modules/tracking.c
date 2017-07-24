#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "ptime.h"
#include "config.h"
#include "ublox.h"
#include "bme280.h"
#include "padc.h"
#include "pac1720.h"
#include "radio.h"
#include "flash.h"
#include "watchdog.h"

static trackPoint_t trackPoints[2];
static trackPoint_t* lastTrackPoint;
static systime_t nextLogEntryTimer;
static module_conf_t trac_conf = {.name = "TRAC"}; // Fake config needed for watchdog tracking
static bool blockThread = true;

/**
  * Returns most recent track point witch is complete.
  */
trackPoint_t* getLastTrackPoint(void)
{
	return lastTrackPoint;
}

/**
  * Returns next free log entry address in memory. Returns 0 if all cells are
  * filled with data
  */
static uint32_t getNextFreeLogAddress(void)
{
	// Search in flash sector 10
	for(uint32_t address = LOG_FLASH_ADDR1; address < LOG_FLASH_ADDR1+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id == 0xFFFFFFFF)
			return address;
	}

	// Search in flash sector 11
	for(uint32_t address = LOG_FLASH_ADDR2; address < LOG_FLASH_ADDR2+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id == 0xFFFFFFFF)
			return address;
	}
	return 0;
}

/**
  * Returns next free log entry address in memory. Returns 0 if all cells are
  * filled with data
  */
static bool getLastLog(trackPoint_t* last)
{
	TRACE_INFO("TRAC > Read last track point from flash memory");

	uint64_t last_time = 0;
	uint32_t last_address = 0;

	// Search in flash sector 10
	for(uint32_t address = LOG_FLASH_ADDR1; address < LOG_FLASH_ADDR1+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id != 0xFFFFFFFF && date2UnixTimestamp(pt.time) >= last_time) {
			last_address = address;
			last_time = date2UnixTimestamp(pt.time);
		}
	}

	// Search in flash sector 11
	for(uint32_t address = LOG_FLASH_ADDR2; address < LOG_FLASH_ADDR2+LOG_SECTOR_SIZE; address += sizeof(trackPoint_t))
	{
		trackPoint_t pt;
		flashRead(address, (char*)&pt, sizeof(trackPoint_t));
		if(pt.id != 0xFFFFFFFF && date2UnixTimestamp(pt.time) >= last_time) {
			last_address = address;
			last_time = date2UnixTimestamp(pt.time);
		}
	}

	if(last_address) {
		flashRead(last_address, (char*)last, sizeof(trackPoint_t));
		TRACE_INFO("TRAC > Found track point in flash memory ID=%d", last->id);
		return true;
	} else {
		TRACE_INFO("TRAC > No track point found in flash memory");
		return false;
	}
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
	uint32_t address = getNextFreeLogAddress();
	if(!address) // Memory completly used, erase oldest data
	{
		eraseOldestLogData();
		address = getNextFreeLogAddress();
	}
	if(!address) // Something went wront at erasing the memory
	{
		TRACE_ERROR("TRAC > Erasing flash failed");
		return;
	}

	// Write data into flash
	TRACE_INFO("TRAC > Flash write (ADDR=%08x)", address);
	flashSectorBegin(flashSectorAt(address));
	flashWrite(address, (char*)tp, sizeof(trackPoint_t));
	flashSectorEnd(flashSectorAt(address));

	// Verify
	if(flashCompare(address, (char*)tp, sizeof(trackPoint_t)))
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
	lastTrackPoint = &trackPoints[0]; // FIXME: That might not work

	// Block thread while no position thread is started
	while(blockThread)
	{
		trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(5);
		chThdSleepMilliseconds(1000);
	}

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

	// Get last GPS fix from memory
	trackPoint_t lastLogPoint;
	if(getLastLog(&lastLogPoint)) { // If there has been stored a trackpoint, then get the last know GPS fix
		lastTrackPoint->gps_lat = lastLogPoint.gps_lat;
		lastTrackPoint->gps_lon = lastLogPoint.gps_lon;
		lastTrackPoint->gps_alt = lastLogPoint.gps_alt;
	}

	lastTrackPoint->gps_lock = 0; // But tell the user that there is no current lock nor any GPS sats locked
	lastTrackPoint->gps_sats = 0;
	lastTrackPoint->gps_ttff = 0;

	// Debug last stored GPS position
	if(lastLogPoint.id != 0xFFFFFFFF) {
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
	lastTrackPoint->adc_isol = pac1720_getIsol();
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

	systime_t time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("TRAC > Do module TRACKING MANAGER cycle");
		trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		trackPoint_t* tp = &trackPoints[id % (sizeof(trackPoints) / sizeof(trackPoint_t))]; // Current track point
		trackPoint_t* ltp = &trackPoints[(id-1) % (sizeof(trackPoints) / sizeof(trackPoint_t))]; // Last track point

		// Search for GPS satellites
		gpsFix_t gpsFix = {{0,0,0,0,0,0,0},0,0,0,0,0};

		// Switch on GPS is enough power is available
		uint16_t batt = getBatteryVoltageMV();
		if(batt >= GPS_ON_VBAT)
		{
			// Switch on GPS
			GPS_Init();

			// Search for lock as long enough power is available
			do {
				batt = getBatteryVoltageMV();
				gps_get_fix(&gpsFix);
			} while(!isGPSLocked(&gpsFix) && batt >= GPS_OFF_VBAT && chVTGetSystemTimeX() <= time + S2ST(TRACK_CYCLE_TIME-5)); // Do as long no GPS lock and within timeout, timeout=cycle-1sec (-1sec in order to keep synchronization)

			if(batt < GPS_OFF_VBAT) // Switch off GPS at low batt
				GPS_Deinit();
		}

		if(isGPSLocked(&gpsFix)) { // GPS locked

			// Switch off GPS (if cycle time is more than 60 seconds)
			#if TRACK_CYCLE_TIME > 60
			GPS_Deinit();
			#endif

			// Debug
			TRACE_INFO("TRAC > GPS sampling finished GPS LOCK");
			TRACE_GPSFIX(&gpsFix);

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

			tp->gps_lock = isGPSLocked(&gpsFix);
			tp->gps_sats = gpsFix.num_svs;

		} else { // GPS lost (keep GPS switched on)

			// Debug
			TRACE_WARN("TRAC > GPS sampling finished GPS LOSS");

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

			// Mark gpsloss
			tp->gps_lock = false;
			tp->gps_sats = 0;

		}

		tp->id = id; // Serial ID
		tp->gps_ttff = ST2S(chVTGetSystemTimeX() - time); // Time to first fix

		// Power management
		tp->adc_vsol = getSolarVoltageMV();
		tp->adc_vbat = getBatteryVoltageMV();
		tp->adc_vusb = getUSBVoltageMV();
		tp->adc_isol = pac1720_getIsol();
		tp->adc_pbat = pac1720_getAvgPbat();

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

		// Trace data
		TRACE_INFO(	"TRAC > New tracking point available (ID=%d)\r\n"
					"%s Time %04d-%02d-%02d %02d:%02d:%02d\r\n"
					"%s Pos  %d.%07d %d.%07d Alt %dm\r\n"
					"%s Sats %d  TTFF %dsec\r\n"
					"%s ADC Vbat=%d.%03dV Vsol=%d.%03dV VUSB=%d.%03dV Pbat=%dmW Isol=%dmA\r\n"
					"%s AIR p=%6d.%01dPa T=%2d.%02ddegC phi=%2d.%01d%%",
					tp->id,
					TRACE_TAB, tp->time.year, tp->time.month, tp->time.day, tp->time.hour, tp->time.minute, tp->time.day,
					TRACE_TAB, tp->gps_lat/10000000, (tp->gps_lat > 0 ? 1:-1)*tp->gps_lat%10000000, tp->gps_lon/10000000, (tp->gps_lon > 0 ? 1:-1)*tp->gps_lon%10000000, tp->gps_alt,
					TRACE_TAB, tp->gps_sats, tp->gps_ttff,
					TRACE_TAB, tp->adc_vbat/1000, (tp->adc_vbat%1000), tp->adc_vsol/1000, (tp->adc_vsol%1000), tp->adc_vusb/1000, (tp->adc_vusb%1000), tp->adc_pbat, tp->adc_isol,
					TRACE_TAB, tp->air_press/10, tp->air_press%10, tp->air_temp/100, tp->air_temp%100, tp->air_hum/10, tp->air_hum%10
		);

		// Append logging (timeout)
		if(nextLogEntryTimer <= chVTGetSystemTimeX() && isGPSLocked(&gpsFix))
		{
			writeLogTrackPoint(tp);
			nextLogEntryTimer += S2ST(LOG_CYCLE_TIME);
		}

		// Switch last recent track point
		lastTrackPoint = tp;
		id++;

		time = chThdSleepUntilWindowed(time, time + S2ST(TRACK_CYCLE_TIME)); // Wait until time + cycletime
	}
}

void init_tracking_manager(void)
{
	TRACE_INFO("TRAC > Startup tracking thread");
	thread_t *th = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(2*1024), "TRA", NORMALPRIO, trackingThread, NULL);
	if(!th) {
		// Print startup error, do not start watchdog for this thread
		TRACE_ERROR("TRAC > Could not startup thread (not enough memory available)");
	} else {
		register_thread_at_wdg(&trac_conf);
		trac_conf.wdg_timeout = chVTGetSystemTimeX() + S2ST(1);
	}
}

/**
  * Without position module, GPS sampling would be a waste a energy, so it
  * should be kept switched off. Due to this reason the tracking manager thread
  * is blocked by the blockThread variable. This prevents the tracking manager
  * to run when no position module is enabled. This function should be called
  * as one position thread is active. This function may be called multiple times.
  */
void setTrackingManagerRunning(void)
{
	blockThread = false;
}
