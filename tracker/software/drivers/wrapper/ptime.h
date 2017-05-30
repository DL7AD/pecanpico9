#ifndef __TIME_H__
#define __TIME_H__

#include "ch.h"
#include "hal.h"

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint16_t millisecond;
} ptime_t;

uint64_t date2UnixTimestamp(ptime_t time);
ptime_t unixTimestamp2Date(uint64_t time);
void getTime(ptime_t *date);
void setTime(ptime_t date);

#endif

