#include "ptime.h"
#include "debug.h"

const uint16_t nonLeapYear[] = {0,31,59,90,120,151,181,212,243,273,304,334,365};
const uint16_t leapYear[] = {0,31,60,91,121,152,182,213,244,274,305,335,366};

/**
  * Calculates UNIX timestamp from Julian calendar based format.
  * Calculation valid until 2100 due to missing leapyear in 2100.
  * @param time Date to be converted
  * @return UNIX timestamp in milliseconds
  */
uint64_t date2UnixTimestamp(ptime_t time) {
	uint64_t timeC = 0;
	timeC  = time.second;
	timeC += time.minute * 60;
	timeC += time.hour * 3600;
	timeC += (time.day-1) * 86400;

	if(time.year % 4 == 0) { // is leapyear?
		timeC += leapYear[time.month-1] * 86400;
	} else {
		timeC += nonLeapYear[time.month-1] * 86400;
	}

	uint16_t i;
	for(i=1970; i<time.year; i++) {
		if(i % 4 == 0) { // is leapyear?
			timeC += 31622400;
		} else {
			timeC += 31536000;
		}
	}
	return timeC * 1000 + time.millisecond;
}

/**
  * Calculates Julian calendar based date/time from UNIX timestamp.
  * Calculation valid until 2100 due to missing leapyear in 2100.
  * @param UNIX timestamp in milliseconds
  * @return Date in Julian calendar format
  */
ptime_t unixTimestamp2Date(uint64_t time) {
	ptime_t date;
	uint64_t dateRaw = time / 1000;

	date.year = 1970;
	while(true)
	{
		uint32_t secondsInThisYear = date.year % 4 ? 31536000 : 31622400;
		if(dateRaw >= secondsInThisYear) {
			dateRaw -= secondsInThisYear;
			date.year++;
		} else {
			break;
		}
	}

	for(date.month=1; (date.year%4 ? nonLeapYear[date.month] : (uint32_t)(leapYear[date.month])*86400)<=dateRaw; date.month++);
	dateRaw -= (date.year%4 ? nonLeapYear[date.month-1] : leapYear[date.month-1])*86400;

	date.day    = (dateRaw / 86400) + 1;
	date.hour   = (dateRaw % 86400) / 3600;
	date.minute = (dateRaw % 3600) / 60;
	date.second = dateRaw % 60;
	date.millisecond = time % 1000;

	return date;
}

/**
  * Reads the time from the STM32 internal RTC
  * @return Date in Julian calendar format
  */
void getTime(ptime_t *date) {
	RTCDateTime timespec;
	rtcGetTime(&RTCD1, &timespec);

	date->year = timespec.year + 2000;
	date->month = timespec.month;
	date->day = timespec.day;
	date->hour = timespec.millisecond / 3600000;
	date->minute = (timespec.millisecond%3600000) / 60000;
	date->second = (timespec.millisecond / 1000) % 60;
	date->millisecond = timespec.millisecond % 1000;
}

/**
  * Sets the STM32 internal time (RTC)
  * @param date Date in Julian calendar format
  */
void setTime(ptime_t date) {
	RTCDateTime timespec;
	timespec.year = date.year - 2000;
	timespec.month = date.month;
	timespec.day = date.day;
	timespec.millisecond = date.hour * 3600000 + date.minute * 60000 + date.second * 1000 + date.millisecond;

	TRACE_INFO("GPS  > Calibrate RTC");
	PRINT_TIME("RTC");
	rtcSetTime(&RTCD1, &timespec);
}

