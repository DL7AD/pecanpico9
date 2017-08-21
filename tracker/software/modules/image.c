#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "modules.h"
#include "ov2640.h"
#include "ov5640.h"
#include "pi2c.h"
#include "ssdv.h"
#include "aprs.h"
#include "radio.h"
#include "base.h"
#include <string.h>
#include "types.h"
#include "sleep.h"
#include "watchdog.h"
#include "flash.h"

static uint8_t gimage_id; // Global image ID (for all image threads)
mutex_t camera_mtx;

void encode_ssdv(uint8_t *image, uint32_t image_len, module_conf_t* conf, uint8_t image_id, bool redudantTx)
{
	ssdv_t ssdv;
	uint8_t pkt[SSDV_PKT_SIZE];
	uint8_t pkt_base91[BASE91LEN(SSDV_PKT_SIZE-37)];
	uint8_t *b;
	uint32_t bi = 0;
	uint8_t c = SSDV_OK;
	uint16_t packet_count = 0;
	uint16_t i = 0;

	// Count packets
	ssdv_enc_init(&ssdv, SSDV_TYPE_NORMAL, conf->ssdv_conf.callsign, image_id);
	ssdv_enc_set_buffer(&ssdv, pkt);

	while(true)
	{
		while((c = ssdv_enc_get_packet(&ssdv)) == SSDV_FEED_ME)
		{
			b = &image[bi];
			uint8_t r = bi < image_len-128 ? 128 : image_len - bi;
			bi += r;
			if(r <= 0)
				break;
			ssdv_enc_feed(&ssdv, b, r);
		}

		if(c == SSDV_EOI || c != SSDV_OK)
			break;

		packet_count++;
	}

	TRACE_INFO("SSDV > %i packets", packet_count);

	// Init SSDV (FEC at 2FSK, non FEC at APRS)
	bi = 0;
	ssdv_enc_init(&ssdv, SSDV_TYPE_NORMAL, conf->ssdv_conf.callsign, image_id);
	ssdv_enc_set_buffer(&ssdv, pkt);

	while(true)
	{
		conf->wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		while((c = ssdv_enc_get_packet(&ssdv)) == SSDV_FEED_ME)
		{
			b = &image[bi];
			uint8_t r = bi < image_len-128 ? 128 : image_len - bi;
			bi += r;

			if(r <= 0)
			{
				TRACE_ERROR("SSDV > Premature end of file");
				break;
			}
			ssdv_enc_feed(&ssdv, b, r);
		}

		if(c == SSDV_EOI)
		{
			TRACE_INFO("SSDV > ssdv_enc_get_packet said EOI");
			break;
		} else if(c != SSDV_OK) {
			TRACE_ERROR("SSDV > ssdv_enc_get_packet failed: %i", c);
			return;
		}

		// Transmit packet
		radioMSG_t msg;
		msg.freq = getFrequency(&conf->frequency);
		msg.power = conf->power;

		switch(conf->protocol) {
			case PROT_APRS_2GFSK:
			case PROT_APRS_AFSK:
				msg.mod = conf->protocol == PROT_APRS_AFSK ? MOD_AFSK : MOD_2GFSK;
				msg.afsk_conf = &(conf->afsk_conf);
				msg.gfsk_conf = &(conf->gfsk_conf);

				// Deleting buffer
				for(uint16_t t=0; t<256; t++)
					pkt_base91[t] = 0;

				base91_encode(&pkt[1], pkt_base91, sizeof(pkt)-37); // Sync byte, CRC and FEC of SSDV not transmitted
				msg.bin_len = aprs_encode_experimental('I', msg.msg, msg.mod, &conf->aprs_conf, pkt_base91, strlen((char*)pkt_base91));

				// Transmit on radio (keep transmitter switched on if packet spacing=0ms and it isnt the last packet being sent)
				if(redudantTx) transmitOnRadio(&msg, false);
				transmitOnRadio(&msg, conf->packet_spacing != 0 || i == packet_count-1);
				break;

			case PROT_SSDV_2FSK:
				msg.mod = MOD_2FSK;
				msg.fsk_conf = &(conf->fsk_conf);

				memcpy(msg.msg, pkt, sizeof(pkt));
				msg.bin_len = 8*sizeof(pkt);

				if(redudantTx) transmitOnRadio(&msg, false);
				transmitOnRadio(&msg, conf->packet_spacing != 0 || i == packet_count-1);
				break;

			default:
				TRACE_ERROR("IMG  > Unsupported protocol selected for module IMAGE");
		}

		// Packet spacing (delay)
		if(conf->packet_spacing)
			chThdSleepMilliseconds(conf->packet_spacing);

		i++;
	}
}

THD_FUNCTION(imgThread, arg) {
	module_conf_t* conf = (module_conf_t*)arg;

	systime_t time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("IMG  > Do module IMAGE cycle");
		conf->wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		if(!p_sleep(&conf->sleep_conf))
		{
			uint32_t image_len = 0;
			uint8_t *image;

			// Lock camera
			TRACE_INFO("IMG  > Lock camera");
			chMtxLock(&camera_mtx);
			TRACE_INFO("IMG  > Locked camera");

			// Lock RADIO from producing interferences
			TRACE_INFO("IMG  > Lock radio");
			chMtxLock(&interference_mtx);
			TRACE_INFO("IMG  > Locked radio");

			uint8_t tries;
			bool status = false;

			// Detect camera
			if(OV2640_isAvailable()) // OV2640 available
			{
				TRACE_INFO("IMG  > OV2640 found");

				if(conf->ssdv_conf.res == RES_MAX) // Attempt maximum resolution (limited by memory)
				{
					conf->ssdv_conf.res = RES_UXGA; // Try maximum resolution

					do {

						// Init camera
						OV2640_init(&conf->ssdv_conf);

						// Sample data from DCMI through DMA into RAM
						tries = 5; // Try 5 times at maximum
						do { // Try capturing image until capture successful
							status = OV2640_Snapshot2RAM();
						} while(!status && --tries);

						conf->ssdv_conf.res--; // Decrement resolution in next attempt (if status==false)

					} while(OV2640_BufferOverflow() && conf->ssdv_conf.res >= RES_QVGA);

					conf->ssdv_conf.res = RES_MAX; // Revert register

				} else { // Static resolution

					// Init camera
					OV2640_init(&conf->ssdv_conf);

					// Sample data from DCMI through DMA into RAM
					tries = 5; // Try 5 times at maximum
					do { // Try capturing image until capture successful
						status = OV2640_Snapshot2RAM();
					} while(!status && --tries);

				}

				// Switch off camera
				OV2640_deinit();

				// Get image
				image_len = OV2640_getBuffer(&image);
				TRACE_INFO("IMG  > Image size: %d bytes", image_len);

			} else if(OV5640_isAvailable()) { // OV5640 available

				TRACE_INFO("IMG  > OV5640 found");

				if(conf->ssdv_conf.res == RES_MAX) // Attempt maximum resolution (limited by memory)
				{
					conf->ssdv_conf.res = RES_UXGA; // Try maximum resolution

					do {

						// Init camera
						OV5640_init(&conf->ssdv_conf);

						// Sample data from DCMI through DMA into RAM
						tries = 5; // Try 5 times at maximum
						do { // Try capturing image until capture successful
							status = OV5640_Snapshot2RAM();
						} while(!status && --tries);

						conf->ssdv_conf.res--; // Decrement resolution in next attempt (if status==false)

					} while(OV5640_BufferOverflow() && conf->ssdv_conf.res >= RES_QVGA);

					conf->ssdv_conf.res = RES_MAX; // Revert register

				} else { // Static resolution

					// Init camera
					OV5640_init(&conf->ssdv_conf);

					// Sample data from DCMI through DMA into RAM
					tries = 5; // Try 5 times at maximum
					do { // Try capturing image until capture successful
						status = OV5640_Snapshot2RAM();
					} while(!status && --tries);

				}

				// Switch off camera
				OV5640_deinit();

				// Get image
				image_len = OV5640_getBuffer(&image);
				TRACE_INFO("IMG  > Image size: %d bytes", image_len);

			} else { // Camera error

				TRACE_ERROR("IMG  > No camera found");

			}

			// Unlock radio
			TRACE_INFO("IMG  > Unlock radio");
			chMtxUnlock(&interference_mtx);
			TRACE_INFO("IMG  > Unlocked radio");

			// Unlock camera
			TRACE_INFO("IMG  > Unlock camera");
			chMtxUnlock(&camera_mtx);
			TRACE_INFO("IMG  > Unlocked camera");

			// Encode/Transmit SSDV if image sampled successfully
			if(status)
			{
				gimage_id++;
				TRACE_INFO("IMG  > Encode/Transmit SSDV ID=%d", gimage_id-1);
				encode_ssdv(image, image_len, conf, gimage_id-1, conf->ssdv_conf.redundantTx);
			}

		}

		time = waitForTrigger(time, &conf->trigger);
	}
}

void start_image_thread(module_conf_t *conf)
{
	if(conf->init_delay) chThdSleepMilliseconds(conf->init_delay);
	TRACE_INFO("IMG  > Startup image thread");
	chsnprintf(conf->name, sizeof(conf->name), "IMG");
	thread_t *th = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(6*1024), "IMG", NORMALPRIO, imgThread, conf);
	if(!th) {
		// Print startup error, do not start watchdog for this thread
		TRACE_ERROR("IMG  > Could not startup thread (not enough memory available)");
	} else {
		register_thread_at_wdg(conf);
		conf->wdg_timeout = chVTGetSystemTimeX() + S2ST(1);
	}
}

