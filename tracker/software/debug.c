#include "ch.h"
#include "hal.h"
#include "debug.h"
#include <stdlib.h>
#include "config.h"

const SerialConfig uart_config =
{
    115200,     // baud rate
    0,          // CR1 register
    0,          // CR2 register
    0           // CR3 register
};

mutex_t trace_mtx; // Used internal to synchronize multiple chprintf in debug.h

bool debug_on_usb = true;

void debugOnUSB_Off(BaseSequentialStream *chp, int argc, char *argv[])
{
	(void)chp;
	(void)argc;
	(void)argv;
	debug_on_usb = false;
}

void debugOnUSB_On(BaseSequentialStream *chp, int argc, char *argv[])
{
	(void)chp;
	(void)argc;
	(void)argv;
	debug_on_usb = true;
}

void printConfig(BaseSequentialStream *chp, int argc, char *argv[])
{
	if(argc < 1)
	{
		chprintf(chp, "Argument missing!\r\n");
		chprintf(chp, "Argument 1: Id of config!\r\n");
	}

	uint8_t id = atoi(argv[0]);
	chprintf(chp, "Config ID=%d\r\n", id);

	chprintf(chp, "Power: %d\r\n", config[id].power);

	if(config[id].frequency.type == FREQ_STATIC) {
		chprintf(chp, "Frequency: %d Hz\r\n", config[id].frequency.hz);
	} else {
		chprintf(chp, "Frequency: APRS region dependent (currently %d Hz\r\n", getFrequency(&config[id].frequency));
	}

	chprintf(chp, "Protocol: %d\r\n", config[id].protocol);
	chprintf(chp, "Initial Delay: %d\r\n", config[id].init_delay);
	chprintf(chp, "Packet Spacing: %d\r\n", config[id].packet_spacing);
	chprintf(chp, "Sleep config: xx\r\n");
	chprintf(chp, "Trigger config: xx\r\n");

	chprintf(chp, "Modulation config: xx\r\n");

	chprintf(chp, "Protocol config: xx\r\n");

	chprintf(chp, "SSDV config: xx\r\n");

	chprintf(chp, "Watchdog timeout: %d\r\n", config[id].wdg_timeout);




}

