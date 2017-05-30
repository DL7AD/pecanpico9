/**
  * Error Logging module
  * Packets send by this module contain tracking points and error logging.
  * Transmitting tracking points is not implemented yet. Error encoding is
  * already implemented and encoded as described:
  * 
  * Error log encoding
  * ========================================
  * Offset	Size
  * 0		4		Absolute time in sec
  * 4		4		Error packet 1
  * 8		4		Error packet 2
  * 12		4		Error packet 3
  * n*4		4		Error packet n
  * 
  * n is defined by ERRORLOG_SIZE in config.h. If less errors occured than the
  * size of ERRORLOG_SIZE, the error log packet may be shorter.
  * 
  * Error packet encoding
  * ========================================
  * Bit
  * 16...31	Time difference to last error packet (if value == 65535, then time difference 65535sec or longer)
  * 10...15	File ID (see File encoding table)
  *  0...9	Line Number
  * 
  */

#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "modules.h"
#include "base.h"
#include "aprs.h"

// File encoding table
const char* const fileIDs[] = {
	"",				// 0 (no error, used for time padding, if no error occured since 2^16-1 seconds)
	""				// 1 (file not listed below)
	"tracking.c",	// 2
	"position.c",	// 3
	"image.c",		// 4
	"log.c",		// 5
	"ssdv.c",		// 6
	"pi2c.c",		// 7
	"padc.c",		// 8
	"ptime.c",		// 9
	"max.c",		// 10
	"si4464.c",		// 11
	"bme280.c",		// 12
	"pac1720.c",	// 13
	"ov2640.c",		// 14
	"sd.c",			// 15
	"radio.c",		// 16
	"sleep.c",		// 17
	"main.c",		// 18
	"error.c"		// 19
};

static error_t error_log[ERRORLOG_SIZE];	// Error log ring memory
static uint8_t error_id;					// Incremental ID for ring memory

void log_error(char *file, uint16_t line)
{
	if(error_id == ERRORLOG_SIZE)
		return; // No memory left

	// Find file ID
	error_log[error_id].error = 1; // Assign default error file (if file not found)
	for(uint8_t i=1; i<sizeof(fileIDs); i++)
		if(!strcmp(fileIDs[i], file)) {
			error_log[error_id].error = ((i+1) << 10);
			break;
		}

	error_log[error_id].time = ST2S(chVTGetSystemTimeX());
	error_log[error_id].error |= (line & 0x3FF);

	error_id++; // Increment counter
}

static uint32_t pkt[ERRORLOG_SIZE+1];
static uint8_t pkt_base91[BASE91LEN(4*ERRORLOG_SIZE+4)];

THD_FUNCTION(moduleERROR, arg)
{
	module_conf_t* config = (module_conf_t*)arg;

	// Execute Initial delay
	if(config->init_delay)
		chThdSleepMilliseconds(config->init_delay);

	// Print initialization message
	TRACE_INFO("ERR  > Startup module %s", config->name);

	systime_t time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("ERR  > Do module LOG cycle");
		config->last_update = chVTGetSystemTimeX(); // Update Watchdog timer

		if(!p_sleep(&config->sleep_config))
		{
			// Encode message
			pkt[0] = ST2S(chVTGetSystemTimeX());
			for(uint8_t i=0; i<error_id; i++)
			{
				if(i > 0) // Calculate time difference
				{
					uint32_t tdiff = error_log[i].time-error_log[i-1].time; // Time difference to last error
					if(tdiff >= 0xFFFF) // Time overflow
						pkt[i+1] = 0xFFFF << 16;
					else
						pkt[i+1] = tdiff << 16;
				}
				pkt[i+1] |= error_log[i].error;
			}
			uint8_t size = error_id;
			error_id = 0;

			radioMSG_t msg;
			msg.freq = getFrequency(&config->frequency);
			msg.power = config->power;

			switch(config->protocol) {
				case PROT_APRS_2GFSK:
				case PROT_APRS_AFSK:
					msg.mod = config->protocol == PROT_APRS_AFSK ? MOD_AFSK : MOD_2GFSK;
					msg.afsk_config = &(config->afsk_config);
					msg.gfsk_config = &(config->gfsk_config);

					// Deleting buffer
					for(uint8_t t=0; t<sizeof(pkt_base91); t++)
						pkt_base91[t] = 0;

					base91_encode((uint8_t*)pkt, pkt_base91, 4*size+4);
					msg.bin_len = aprs_encode_experimental('E', msg.msg, msg.mod, &config->aprs_config, pkt_base91, strlen((char*)pkt_base91));

					transmitOnRadio(&msg);
					break;

				default:
					TRACE_ERROR("ERR  > Unsupported protocol selected for module ERR");
			}
		}

		time = waitForTrigger(time, &config->trigger);
	}
}

