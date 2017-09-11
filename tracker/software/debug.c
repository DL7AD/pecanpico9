#include "ch.h"
#include "hal.h"
#include "debug.h"
#include <stdlib.h>
#include "config.h"
#include "image.h"
#include "tracking.h"
#include "pi2c.h"

const SerialConfig uart_config =
{
    115200,     // baud rate
    0,          // CR1 register
    0,          // CR2 register
    0           // CR3 register
};

mutex_t trace_mtx; // Used internal to synchronize multiple chprintf in debug.h

bool debug_on_usb = false;

void debugOnUSB(BaseSequentialStream *chp, int argc, char *argv[])
{
	if(argc < 1)
	{
		chprintf(chp, "Argument missing!\r\n");
		chprintf(chp, "Argument 1: 1 for switch on, 0 for switch off!\r\n");
		return;
	}

	debug_on_usb = atoi(argv[0]);
}

static uint8_t usb_buffer[64*1024] __attribute__((aligned(32))); // USB image buffer
void printPicture(BaseSequentialStream *chp, int argc, char *argv[])
{
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
				streamPut(chp, 0xFF);
				streamPut(chp, 0xD8);
			}
			if(start_detected)
				streamPut(chp, conf.ram_buffer[i]);
		}
		if(!start_detected)
		{
			TRACE_USB("DATA > image/jpeg,0");
			TRACE_USB("DATA > text/trace,no SOI flag found");
		}

	} else { // No camera found

		TRACE_USB("DATA > image,jpeg,0");
		TRACE_USB("DATA > error,no camera found");

	}
}

void command2Camera(BaseSequentialStream *chp, int argc, char *argv[])
{
	(void)chp;
	(void)argc;
	I2C_write8_16bitreg(0x3C, atoi(argv[0]), atoi(argv[1]));
}

void readLog(BaseSequentialStream *chp, int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	chprintf(chp, "id,date,time,lat,lon,alt,sats,ttff,vbat,vsol,vsub,pbat,rbat,press,temp,hum,idimg\r\n");

	trackPoint_t *tp;
	for(uint16_t i=0; (tp = getLogBuffer(i)) != NULL; i++)
		if(tp->id != 0xFFFFFFFF)
		{
			chprintf(	chp,
						"%d,%04d-%02d-%02d,%02d:%02d:%02d,%d.%05d,%d.%05d,%d,%d,%d,%d.%03d,%d.%03d,%d.%03d,%d,%d,%d.%01d,%2d.%02d,%2d.%01d,%d\r\n",
						tp->id,tp->time.year, tp->time.month, tp->time.day, tp->time.hour, tp->time.minute, tp->time.day,
						tp->gps_lat/10000000, (tp->gps_lat > 0 ? 1:-1)*(tp->gps_lat/100)%100000, tp->gps_lon/10000000, (tp->gps_lon > 0 ? 1:-1)*(tp->gps_lon/100)%100000, tp->gps_alt,
						tp->gps_sats, tp->gps_ttff,
						tp->adc_vbat/1000, (tp->adc_vbat%1000), tp->adc_vsol/1000, (tp->adc_vsol%1000), tp->adc_vusb/1000, (tp->adc_vusb%1000), tp->adc_pbat, tp->adc_rbat,
						tp->air_press/10, tp->air_press%10, tp->air_temp/100, tp->air_temp%100, tp->air_hum/10, tp->air_hum%10, tp->id_image
			);
		}
}

void printConfig(BaseSequentialStream *chp, int argc, char *argv[])
{
	if(argc < 1)
	{
		chprintf(chp, "Argument missing!\r\n");
		chprintf(chp, "Argument 1: Id of config!\r\n");
		return;
	}

	uint8_t id = atoi(argv[0]);
	chprintf(chp, "Config ID=%d\r\n", id);

	chprintf(chp, "Power: %d\r\n", config[id].power);

	if(config[id].frequency.type == FREQ_STATIC) {
		uint32_t freq = config[id].frequency.hz;
		if((freq/1000)*1000 == freq)
			chprintf(chp, "Frequency: %d.%03d MHz\r\n", freq/1000000, (freq%1000000)/1000);
		else
			chprintf(chp, "Frequency: %d.%03d MHz\r\n", freq/1000000, (freq%1000000));
	} else {
		uint32_t freq = getFrequency(&config[id].frequency);
		chprintf(chp, "Frequency: APRS region dependent (currently %d.%03d MHz)\r\n", freq/1000000, (freq%1000000)/1000);
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

