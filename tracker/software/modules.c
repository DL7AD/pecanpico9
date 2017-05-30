#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "modules.h"

char *SMODE_STRING[] = {
	"ACTIVE", "SLEEP"
};
char *PROTOCOL_STRING[] = {
	"UKHAS 2FSK", "UKHAS DominoEX16", "APRS AFSK 1200", "APRS 2GFSK 9600",
	"APRS TELEMETRY CONFIG AFSK 1200", "APRS TELEMETRY CONFIG 2GFSK 9600",
	"SSDV 2FSK", "SSDV on APRS AFSK 1200", "SSDV on APRS 2GFSK 9600", "MORSE"
};
char *MOULATION_STRING[] = {
	"OOK", "2FSK", "2GFSK 9k6", "DOMINOEX16", "AFSK 1k2"
};

mutex_t interference_mtx;

systime_t watchdog_tracking;

