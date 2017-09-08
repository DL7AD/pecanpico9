#include "ch.h"
#include "hal.h"
#include "debug.h"
#include <stdlib.h>
#include "config.h"
#include "image.h"

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

static uint8_t usb_buffer[96*1024] __attribute__((aligned(32))); // USB image buffer
void printPicture(BaseSequentialStream *chp, int argc, char *argv[])
{
	(void)chp;
	(void)argc;
	(void)argv;

	// Take picture
	ssdv_conf_t conf = {
		.res = RES_VGA,
		.quality = 4,
		.ram_buffer = usb_buffer,
		.ram_size = sizeof(usb_buffer),
	};
	bool camera_found = takePicture(&conf, false);

	// Transmit image via USB
	if(camera_found)
	{

		bool start_detected = false;
		for(uint32_t i=0; i<conf.size_sampled; i++)
		{
			// Look for APP0 instead of SOI because SOI is lost sometimes, but we can add SOI easily later on
			if(!start_detected && conf.ram_buffer[i] == 0xFF && conf.ram_buffer[i+1] == 0xE0) {
				start_detected = true;
				TRACE_USB("DATA > image/jpeg,%d", conf.size_sampled-i+1); // Flag the data on serial output
				streamPut(&SDU1, 0xFF);
				streamPut(&SDU1, 0xD8);
			}
			if(start_detected)
				streamPut(&SDU1, conf.ram_buffer[i]);
		}

	} else { // No camera found

		for(uint32_t i=0; i<sizeof(noCameraFound); i++)
			streamPut(&SDU1, noCameraFound[i]);

	}
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

