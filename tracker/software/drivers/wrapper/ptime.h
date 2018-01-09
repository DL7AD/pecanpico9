#ifndef __PTIME_H__
#define __PTIME_H__

#include "ch.h"

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} ptime_t;

uint32_t date2UnixTimestamp(ptime_t *date);
void unixTimestamp2Date(ptime_t *date, uint32_t time);
void getTime(ptime_t *date);
void setTime(ptime_t *date);

#endif

