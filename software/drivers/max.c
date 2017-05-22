/**
  * @see https://github.com/thasti/utrak
  */

#include "ch.h"
#include "hal.h"
#include "max.h"
#include "pi2c.h"
#include "debug.h"
#include "config.h"
#include "defines.h"

// Serial driver configuration for GPS
const SerialConfig gps_config =
{
	9600,	// baud rate
	0,		// CR1 register
	0,		// CR2 register
	0		// CR3 register
};

/* 
 * gps_transmit_string
 *
 * transmits a command to the GPS
 */
void gps_transmit_string(uint8_t *cmd, uint8_t length)
{
	sdWrite(&SD5, cmd, length);
}

/* 
 * gps_receive_ack
 *
 * waits for transmission of an ACK/NAK message from the GPS.
 *
 * returns 1 if ACK was received, 0 if NAK was received
 *
 */
uint8_t gps_receive_ack(uint8_t class_id, uint8_t msg_id, uint16_t timeout) {
	int match_count = 0;
	int msg_ack = 0;
	uint8_t rx_byte;
	uint8_t ack[] = {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x00, 0x00};
	uint8_t nak[] = {0xB5, 0x62, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00};
	ack[6] = class_id;
	nak[6] = class_id;
	ack[7] = msg_id;
	nak[7] = msg_id;

	// runs until ACK/NAK packet is received
	systime_t sTimeout = chVTGetSystemTimeX() + MS2ST(timeout);
	while(chVTGetSystemTimeX() <= sTimeout) {

		// Receive one byte
		rx_byte = sdGetTimeout(&SD5, sTimeout - chVTGetSystemTimeX());

		// Process one byte
		if (rx_byte == ack[match_count] || rx_byte == nak[match_count]) {
			if (match_count == 3) {	/* test ACK/NAK byte */
				if (rx_byte == ack[match_count]) {
					msg_ack = 1;
				} else {
					msg_ack = 0;
				}
			}
			if (match_count == 7) { 
				return msg_ack;
			}
			match_count++;
		} else {
			match_count = 0;
		}

	}

	return false;
}

/*
 * gps_receive_payload
 *
 * retrieves the payload of a packet with a given class and message-id with the retrieved length.
 * the caller has to ensure suitable buffer length!
 *
 * returns the length of the payload
 *
 */
uint16_t gps_receive_payload(uint8_t class_id, uint8_t msg_id, unsigned char *payload, uint16_t timeout) {
	uint8_t rx_byte;
	enum {UBX_A, UBX_B, CLASSID, MSGID, LEN_A, LEN_B, PAYLOAD} state = UBX_A;
	uint16_t payload_cnt = 0;
	uint16_t payload_len = 0;

	systime_t sTimeout = chVTGetSystemTimeX() + MS2ST(timeout);
	while(chVTGetSystemTimeX() <= sTimeout) {

		// Receive one byte
		rx_byte = sdGetTimeout(&SD5, sTimeout - chVTGetSystemTimeX());

		// Process one byte
		switch (state) {
			case UBX_A:
				if (rx_byte == 0xB5)	state = UBX_B;
				else 			state = UBX_A;
				break;
			case UBX_B:
				if (rx_byte == 0x62)	state = CLASSID;
				else			state = UBX_A;
				break;
			case CLASSID:
				if (rx_byte == class_id)state = MSGID;
				else			state = UBX_A;
				break;
			case MSGID:
				if (rx_byte == msg_id)	state = LEN_A;
				else			state = UBX_A;
				break;
			case LEN_A:
				payload_len = rx_byte;
				state = LEN_B;
				break;
			case LEN_B:
				payload_len |= ((uint16_t)rx_byte << 8);
				state = PAYLOAD;
				break;
			case PAYLOAD:
				payload[payload_cnt] = rx_byte;
				payload_cnt++;
				if (payload_cnt == payload_len)
					return payload_len;
				break;
			default:
				state = UBX_A;
		}
	}

	return 0;
}

/* 
 * gps_get_fix
 *
 * retrieves a GPS fix from the module. if validity flag is not set, date/time and position/altitude are 
 * assumed not to be reliable!
 *
 * This method divides MAX7/8 and MAX6 modules since the protocol changed at MAX7 series. MAX6 requires
 * NAV-POSLLH NAV-TIMEUTC and NAV-SOL to get all information about the GPS. With implementation of the
 * NAV-PVT message at the MAX7 series, all information can be aquired by only one message. Although
 * MAX7 is backward compatible, MAX7/8 will use NAV-PVT rather than the old protocol.
 *
 * argument is call by reference to avoid large stack allocations
 *
 */
bool gps_get_fix(gpsFix_t *fix) {
	static uint8_t response[92];

	// Transmit request
	uint8_t pvt[] = {0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19};
	gps_transmit_string(pvt, sizeof(pvt));

	if(!gps_receive_payload(0x01, 0x07, response, 5000)) { // Receive request
		TRACE_INFO("GPS  > PVT Polling FAILED");
		return false;
	}

	TRACE_INFO("GPS  > PVT Polling OK");

	fix->num_svs = response[23];
	fix->type = response[20];

	fix->time.year = response[4] + (response[5] << 8);
	fix->time.month = response[6];
	fix->time.day = response[7];
	fix->time.hour = response[8];
	fix->time.minute = response[9];
	fix->time.second = response[10];

	fix->lat = (int32_t) (
			(uint32_t)(response[28]) + ((uint32_t)(response[29]) << 8) + ((uint32_t)(response[30]) << 16) + ((uint32_t)(response[31]) << 24)
			);
	fix->lon = (int32_t) (
			(uint32_t)(response[24]) + ((uint32_t)(response[25]) << 8) + ((uint32_t)(response[26]) << 16) + ((uint32_t)(response[27]) << 24)
			);
	int32_t alt_tmp = (((int32_t) 
			((uint32_t)(response[36]) + ((uint32_t)(response[37]) << 8) + ((uint32_t)(response[38]) << 16) + ((uint32_t)(response[39]) << 24))
			) / 1000);
	if (alt_tmp <= 0) {
		fix->alt = 1;
	} else if (alt_tmp > 50000) {
		fix->alt = 50000;
	} else {
		fix->alt = (uint16_t)alt_tmp;
	}

	return true;
}

/* 
 * gps_disable_nmea_output
 *
 * disables all NMEA messages to be output from the GPS.
 * even though the parser can cope with NMEA messages and ignores them, it 
 * may save power to disable them completely.
 *
 * returns if ACKed by GPS
 *
 */
uint8_t gps_disable_nmea_output(void) {
	uint8_t nonmea[] = {
		0xB5, 0x62, 0x06, 0x00, 20, 0x00,	// UBX-CFG-PRT
		0x01, 0x00, 0x00, 0x00, 			// UART1, reserved, no TX ready
		0xe0, 0x08, 0x00, 0x00,				// UART mode (8N1)
		0x80, 0x25, 0x00, 0x00,				// UART baud rate (9600)
		0x01, 0x00,							// input protocols (uBx only)
		0x01, 0x00,							// output protocols (uBx only)
		0x00, 0x00,							// flags
		0x00, 0x00,							// reserved
		0xaa, 0x79							// checksum
	};

	gps_transmit_string(nonmea, sizeof(nonmea));
	return gps_receive_ack(0x06, 0x00, 1000);
}

/*
 * gps_set_airborne_model
 *
 * tells the GPS to use the airborne positioning model. Should be used to
 * get stable lock up to 50km altitude
 *
 * working uBlox MAX-M8Q
 *
 * returns if ACKed by GPS
 *
 */
uint8_t gps_set_airborne_model(void) {
	uint8_t model6[] = {
		0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 	// UBX-CFG-NAV5
		0xFF, 0xFF, 							// parameter bitmask
		0x06, 									// dynamic model
		0x03, 									// fix mode
		0x00, 0x00, 0x00, 0x00, 				// 2D fix altitude
		0x10, 0x27, 0x00, 0x00,					// 2D fix altitude variance
		0x05, 									// minimum elevation
		0x00, 									// reserved
		0xFA, 0x00, 							// position DOP
		0xFA, 0x00, 							// time DOP
		0x64, 0x00, 							// position accuracy
		0x2C, 0x01, 							// time accuracy
		0x00,									// static hold threshold 
		0x3C, 									// DGPS timeout
		0x00, 									// min. SVs above C/No thresh
		0x00, 									// C/No threshold
		0x00, 0x00, 							// reserved
		0xc8, 0x00,								// static hold max. distance
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	// reserved
		0x1a, 0x28								// checksum
	};

	gps_transmit_string(model6, sizeof(model6));
	return gps_receive_ack(0x06, 0x24, 1000);
}

/*
 * gps_set_power_save
 *
 * enables cyclic tracking on the uBlox M8Q
 *
 * returns if ACKed by GPS
 *
 */
uint8_t gps_set_power_save(void) {
	uint8_t powersave[] = {
		0xB5, 0x62, 0x06, 0x3B, 44, 0,		// UBX-CFG-PM2
		0x01, 0x00, 0x00, 0x00, 			// v1, reserved 1..3
		0x00, 0b00010000, 0b00000010, 0x00,	// cyclic tracking, update ephemeris
		0x10, 0x27, 0x00, 0x00,				// update period, ms
		0x10, 0x27, 0x00, 0x00,				// search period, ms
		0x00, 0x00, 0x00, 0x00,				// grid offset
		0x00, 0x00,							// on-time after first fix
		0x01, 0x00,							// minimum acquisition time
		0x00, 0x00, 0x00, 0x00,				// reserved 4,5
		0x00, 0x00, 0x00, 0x00,				// reserved 6
		0x00, 0x00, 0x00, 0x00,				// reserved 7
		0x00, 0x00, 0x00, 0x00,				// reserved 8,9,10
		0x00, 0x00, 0x00, 0x00,				// reserved 11
		0xef, 0x29
	};

	gps_transmit_string(powersave, sizeof(powersave));
	return gps_receive_ack(0x06, 0x3B, 1000);
}

/*
 * gps_power_save
 *
 * enables or disables the power save mode (which was configured before)
 */
uint8_t gps_power_save(int on) {
	uint8_t recvmgmt[] = {
		0xB5, 0x62, 0x06, 0x11, 2, 0,	// UBX-CFG-RXM
		0x08, 0x01,						// reserved, enable power save mode
		0x22, 0x92
	};
	if (!on) {
		recvmgmt[7] = 0x00;		// continuous mode
		recvmgmt[8] = 0x21;		// new checksum
		recvmgmt[9] = 0x91;
	}

	gps_transmit_string(recvmgmt, sizeof(recvmgmt));
	return gps_receive_ack(0x06, 0x11, 1000);
}

bool GPS_Init(void) {
	// Initialize pins
	TRACE_INFO("GPS  > Init pins");
	palSetPadMode(PORT(GPS_RESET), PIN(GPS_RESET), PAL_MODE_OUTPUT_PUSHPULL);	// GPS reset
	palSetPadMode(PORT(GPS_EN), PIN(GPS_EN), PAL_MODE_OUTPUT_PUSHPULL);			// GPS off
	palSetPadMode(PORT(GPS_TIMEPULSE), PIN(GPS_TIMEPULSE), PAL_MODE_INPUT);		// GPS timepulse
	palSetPadMode(PORT(GPS_RXD), PIN(GPS_RXD), PAL_MODE_ALTERNATE(8));			// UART RXD
	palSetPadMode(PORT(GPS_TXD), PIN(GPS_TXD), PAL_MODE_ALTERNATE(8));			// UART TXD

	// Init UART
	TRACE_INFO("GPS  > Init GPS UART");
	sdStart(&SD5, &gps_config);

	// Switch MOSFET
	TRACE_INFO("GPS  > Switch on");
	palSetPad(PORT(GPS_RESET), PIN(GPS_RESET));	// Pull up GPS reset
	palSetPad(PORT(GPS_EN), PIN(GPS_EN));		// Switch on GPS
	
	// Wait for GPS startup
	chThdSleepMilliseconds(3000);

	uint8_t status = 1;

	// Configure GPS
	TRACE_INFO("GPS  > Initialize GPS");
	if(gps_disable_nmea_output()) {
		TRACE_INFO("GPS  > Disable NMEA output OK");
	} else {
		TRACE_ERROR("GPS  > Disable NMEA output FAILED");
		status = 0;
	}

	if(gps_set_airborne_model()) {
		TRACE_INFO("GPS  > Set airborne model OK");
	} else {
		TRACE_ERROR("GPS  > Set airborne model FAILED");
		status = 0;
	}
	if(gps_set_power_save()) {
		TRACE_INFO("GPS  > Configure power save OK");
	} else {
		TRACE_ERROR("GPS  > Configure power save FAILED");
		status = 0;
	}
	if(gps_power_save(0)) {
		TRACE_INFO("GPS  > Disable power save OK");
	} else {
		TRACE_ERROR("GPS  > Disable power save FAILED");
		status = 0;
	}

	return status;
}

void GPS_Deinit(void)
{
	// Switch MOSFET
	TRACE_INFO("GPS  > Switch off");
	palClearPad(PORT(GPS_EN), PIN(GPS_EN));

	// Deinit pins
	palSetPadMode(PORT(GPS_RESET), PIN(GPS_RESET), PAL_MODE_INPUT);	// GPS reset
	palSetPadMode(PORT(GPS_RXD), PIN(GPS_RXD), PAL_MODE_INPUT);		// UART RXD
	palSetPadMode(PORT(GPS_TXD), PIN(GPS_TXD), PAL_MODE_INPUT);		// UART TXD
}

/**
  * Returns accurate MCU frequency in 40Hz steps. The frequency is measured by
  * the GPS timepulse. So it requires GPS lock. This function will timeout
  * after 32000000 cycles if GPS is not locked or switched off.
  */
uint32_t GPS_get_mcu_frequency(void)
{
	uint32_t i = 0;
	uint32_t timeout = 32000000;

	// Lock RTOS
	chSysLock();

	// Sync
	while( palReadPad(PORT(GPS_TIMEPULSE), PIN(GPS_TIMEPULSE)) && timeout-- );
	while( !palReadPad(PORT(GPS_TIMEPULSE), PIN(GPS_TIMEPULSE)) && timeout-- );

	// Count clocks (9 clocks each cycle)
	while( palReadPad(PORT(GPS_TIMEPULSE), PIN(GPS_TIMEPULSE)) && timeout-- ) // Loop takes 100ms
		i++;
	while( !palReadPad(PORT(GPS_TIMEPULSE), PIN(GPS_TIMEPULSE)) && timeout-- ) // Loop takes 900ms
		i++;

	// Unlock RTOS
	chSysUnlock();

	return i*9;
}

