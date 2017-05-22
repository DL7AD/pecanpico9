#ifndef __GEOFENCE_H__
#define __GEOFENCE_H__

#include "ch.h"
#include "hal.h"

typedef struct {
	int32_t lat;
	int32_t lon;
} coord_t;

bool isPointInPolygon(const coord_t *poly, uint32_t size, int32_t lat, int32_t lon);
bool isPointInAmerica(int32_t lat, int32_t lon);
bool isPointInChina(int32_t lat, int32_t lon);
bool isPointInJapan(int32_t lat, int32_t lon);
bool isPointInSouthkorea(int32_t lat, int32_t lon);
bool isPointInSoutheastAsia(int32_t lat, int32_t lon);
bool isPointInAustralia(int32_t lat, int32_t lon);
bool isPointInNewZealand(int32_t lat, int32_t lon);
bool isPointInArgentina(int32_t lat, int32_t lon);
bool isPointInBrazil(int32_t lat, int32_t lon);

#endif
