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
uint32_t date2UnixTimestamp(ptime_t *date) {
	uint32_t timeC = 0;
	timeC  = date->second;
	timeC += date->minute * 60;
	timeC += date->hour * 3600;
	timeC += (date->day-1) * 86400;

	if(date->year % 4 == 0) { // is leapyear?
		timeC += leapYear[date->month-1] * 86400;
	} else {
		timeC += nonLeapYear[date->month-1] * 86400;
	}

	for(uint16_t i=1970; i<date->year; i++) {
		if(i % 4 == 0) { // is leapyear?
			timeC += 31622400;
		} else {
			timeC += 31536000;
		}
	}
	return timeC;
}

/**
  * Calculates Julian calendar based date/time from UNIX timestamp.
  * Calculation valid until 2100 due to missing leapyear in 2100.
  * @param UNIX timestamp in milliseconds
  * @return Date in Julian calendar format
  */
void unixTimestamp2Date(ptime_t *date, uint32_t time) {
	date->year = 1970;
	while(true)
	{
		uint32_t secondsInThisYear = date->year % 4 ? 31536000 : 31622400;
		if(time >= secondsInThisYear) {
			time -= secondsInThisYear;
			date->year++;
		} else {
			break;
		}
	}

	for(date->month=1; (date->year%4 ? nonLeapYear[date->month]*86400 : leapYear[date->month]*86400)<=time; date->month++);
	time -= (date->year%4 ? nonLeapYear[date->month-1] : leapYear[date->month-1])*86400;

	date->day    = (time / 86400) + 1;
	date->hour   = (time % 86400) / 3600;
	date->minute = (time % 3600) / 60;
	date->second = time % 60;
}

/**
  * Reads the time from the STM32 internal RTC
  * @return Date in Julian calendar format
  */
void getTime(ptime_t *date) {
	TRACE_INFO("GPS  > Get time from RTC");
	RTCDateTime timespec;
	rtcGetTime(&RTCD1, &timespec);

	date->year = timespec.year + 2000;
	date->month = timespec.month;
	date->day = timespec.day;
	date->hour = timespec.millisecond / 3600000;
	date->minute = (timespec.millisecond%3600000) / 60000;
	date->second = (timespec.millisecond / 1000) % 60;
}

/**
  * Sets the STM32 internal time (RTC)
  * @param date Date in Julian calendar format
  */
void setTime(ptime_t *date) {
	TRACE_INFO("GPS  > Calibrate RTC");
	RTCDateTime timespec;
	timespec.year = date->year - 2000;
	timespec.month = date->month;
	timespec.day = date->day;
	timespec.millisecond = date->hour * 3600000 + date->minute * 60000 + date->second * 1000;

	rtcSetTime(&RTCD1, &timespec);
}

