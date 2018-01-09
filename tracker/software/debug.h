#ifndef __TRACE_H__
#define __TRACE_H__

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "ptime.h"
#include "config.h"
#include <string.h>
#include "usbcfg.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

extern mutex_t trace_mtx;
extern const SerialConfig uart_config;
extern bool debug_on_usb;

// Initializer for serial debug and LEDs
#define DEBUG_INIT() { \
	palSetLineMode(LINE_IO_LED1, PAL_MODE_OUTPUT_PUSHPULL); \
	palSetLineMode(LINE_IO_LED2, PAL_MODE_OUTPUT_PUSHPULL); \
	\
	sdStart(&SD3, &uart_config); \
	palSetLineMode(LINE_IO_TXD, PAL_MODE_ALTERNATE(7)); \
	chMtxObjectInit(&trace_mtx); \
	chMtxLock(&trace_mtx); \
	chprintf((BaseSequentialStream*)&SD3, "\r\n"); \
	chMtxUnlock(&trace_mtx); \
}

#define TRACE_BASE(format, type, args...) { \
	chMtxLock(&trace_mtx); \
	\
	if(TRACE_TIME) { \
		chprintf((BaseSequentialStream*)&SD3, "[%8d.%03d]", chVTGetSystemTimeX()/CH_CFG_ST_FREQUENCY, (chVTGetSystemTimeX()*1000/CH_CFG_ST_FREQUENCY)%1000); \
	} \
	chprintf((BaseSequentialStream*)&SD3, "[%s]", type); \
	if(TRACE_FILE) { \
		chprintf((BaseSequentialStream*)&SD3, "[%10s %04d]", __FILENAME__, __LINE__); \
	} \
	chprintf((BaseSequentialStream*)&SD3, " "); \
	chprintf((BaseSequentialStream*)&SD3, (format), ##args); \
	chprintf((BaseSequentialStream*)&SD3, "\r\n"); \
	\
	if(debug_on_usb) \
		TRACE_BASE_USB(format, type, ##args); \
	chMtxUnlock(&trace_mtx); \
}

#define TRACE_BASE_USB(format, type, args...) { \
	if(usb_initialized) { \
		if(TRACE_TIME) { \
			chprintf((BaseSequentialStream*)&SDU1, "[%8d.%03d]", chVTGetSystemTimeX()/CH_CFG_ST_FREQUENCY, (chVTGetSystemTimeX()*1000/CH_CFG_ST_FREQUENCY)%1000); \
		} \
		chprintf((BaseSequentialStream*)&SDU1, "[%s]", type); \
		if(TRACE_FILE) { \
			chprintf((BaseSequentialStream*)&SDU1, "[%10s %04d]", __FILENAME__, __LINE__); \
		} \
		chprintf((BaseSequentialStream*)&SDU1, " "); \
		chprintf((BaseSequentialStream*)&SDU1, (format), ##args); \
		chprintf((BaseSequentialStream*)&SDU1, "\r\n"); \
	} \
}

#define TRACE_DEBUG(format, args...) TRACE_BASE(format, "DEBUG", ##args)
#define TRACE_INFO(format, args...)  TRACE_BASE(format, "     ", ##args)
#define TRACE_WARN(format, args...)  TRACE_BASE(format, "WARN ", ##args)
#define TRACE_ERROR(format, args...) TRACE_BASE(format, "ERROR", ##args)
#define TRACE_USB(format, args...)   TRACE_BASE_USB(format, "USB  ", ##args) /* only traced on USB */

#if TRACE_TIME && TRACE_FILE
#define TRACE_TAB "                                             "
#elif TRACE_TIME && !TRACE_FILE
#define TRACE_TAB "                            "
#elif !TRACE_TIME && TRACE_FILE
#define TRACE_TAB "                               "
#else
#define TRACE_TAB "              "
#endif

#define TRACE_BIN(data, len) { \
	chMtxLock(&trace_mtx); \
	chprintf((BaseSequentialStream*)&SD3, "[%8d.%03d][DEBUG] ", chVTGetSystemTimeX()/CH_CFG_ST_FREQUENCY, (chVTGetSystemTimeX()*1000/CH_CFG_ST_FREQUENCY)%1000); \
	chprintf((BaseSequentialStream*)&SD3, "     > Binary data (%d bits)\r\n", (len)); \
	for(uint32_t i=0; i<((len)+7)/8; i+=8) \
		chprintf((BaseSequentialStream*)&SD3, "%s 0x%03x ... 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\r\n", \
		TRACE_TAB, i, (data)[i], (data)[i+1], (data)[i+2], (data)[i+3], (data)[i+4], (data)[i+5], (data)[i+6], (data)[i+7]); \
	chMtxUnlock(&trace_mtx); \
}

#define TRACE_BIN_CHAR(data, len) { \
	chMtxLock(&trace_mtx); \
	chprintf((BaseSequentialStream*)&SD3, "[%8d.%03d][DEBUG] ", chVTGetSystemTimeX()/CH_CFG_ST_FREQUENCY, (chVTGetSystemTimeX()*1000/CH_CFG_ST_FREQUENCY)%1000); \
	chprintf((BaseSequentialStream*)&SD3, "     > Binary data (%d bits)\r\n", (len)); \
	for(uint32_t i=0; i<((len)+7)/8; i+=8) \
		chprintf((BaseSequentialStream*)&SD3, "%s %c%c%c%c%c%c%c%c\r\n", \
		TRACE_TAB, i, (data)[i], (data)[i+1], (data)[i+2], (data)[i+3], (data)[i+4], (data)[i+5], (data)[i+6], (data)[i+7]); \
	chMtxUnlock(&trace_mtx); \
}

void debugOnUSB(BaseSequentialStream *chp, int argc, char *argv[]);
void printConfig(BaseSequentialStream *chp, int argc, char *argv[]);
void printPicture(BaseSequentialStream *chp, int argc, char *argv[]);
void readLog(BaseSequentialStream *chp, int argc, char *argv[]);
void command2Camera(BaseSequentialStream *chp, int argc, char *argv[]);

#endif

