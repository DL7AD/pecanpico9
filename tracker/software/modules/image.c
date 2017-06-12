#include "ch.h"
#include "hal.h"

#include "debug.h"
#include "modules.h"
#include "ov2640.h"
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

void encode_ssdv(uint8_t *image, uint32_t image_len, module_conf_t* config, uint8_t image_id)
{
	ssdv_t ssdv;
	uint8_t pkt[SSDV_PKT_SIZE];
	uint8_t pkt_base91[BASE91LEN(SSDV_PKT_SIZE-37)];
	uint16_t i = 0;
	uint8_t *b;
	uint32_t bi = 0;
	uint8_t c = SSDV_OK;

	// Init SSDV (FEC at 2FSK, non FEC at APRS)
	ssdv_enc_init(&ssdv, SSDV_TYPE_NORMAL, config->ssdv_config.callsign, image_id);
	ssdv_enc_set_buffer(&ssdv, pkt);

	while(true)
	{
		config->wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

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
		msg.freq = getFrequency(&config->frequency);
		msg.power = config->power;

		switch(config->protocol) {
			case PROT_APRS_2GFSK:
			case PROT_APRS_AFSK:
				msg.mod = config->protocol == PROT_APRS_AFSK ? MOD_AFSK : MOD_2GFSK;
				msg.afsk_config = &(config->afsk_config);
				msg.gfsk_config = &(config->gfsk_config);

				// Deleting buffer
				for(uint16_t t=0; t<256; t++)
					pkt_base91[t] = 0;

				base91_encode(&pkt[1], pkt_base91, sizeof(pkt)-37); // Sync byte, CRC and FEC of SSDV not transmitted
				msg.bin_len = aprs_encode_experimental('I', msg.msg, msg.mod, &config->aprs_config, pkt_base91, strlen((char*)pkt_base91));

				transmitOnRadio(&msg);
				break;

			case PROT_SSDV_2FSK:
				msg.mod = MOD_2FSK;
				msg.fsk_config = &(config->fsk_config);

				memcpy(msg.msg, pkt, sizeof(pkt));
				msg.bin_len = 8*sizeof(pkt);

				transmitOnRadio(&msg);
				break;

			default:
				TRACE_ERROR("IMG  > Unsupported protocol selected for module IMAGE");
		}

		// Packet spacing (delay)
		if(config->packet_spacing)
			chThdSleepMilliseconds(config->packet_spacing);

		i++;
	}

	TRACE_INFO("SSDV > %i packets", i);
}

THD_FUNCTION(imgThread, arg) {
	module_conf_t* config = (module_conf_t*)arg;

	systime_t time = chVTGetSystemTimeX();
	while(true)
	{
		TRACE_INFO("IMG  > Do module IMAGE cycle");
		config->wdg_timeout = chVTGetSystemTimeX() + S2ST(600); // TODO: Implement more sophisticated method

		if(!p_sleep(&config->sleep_config))
		{
			uint32_t image_len = 0;
			uint8_t *image;

			// Take photo if camera activated (if camera disabled, camera buffer is probably shared in config file)
			if(!config->ssdv_config.no_camera)
			{
				// Lock camera
				TRACE_INFO("IMG  > Lock camera");
				chMtxLock(&camera_mtx);
				TRACE_INFO("IMG  > Locked camera");

				// Lock RADIO from producing interferences
				TRACE_INFO("IMG  > Lock radio");
				chMtxLock(&interference_mtx);
				TRACE_INFO("IMG  > Locked radio");

				// Shutdown radio (to avoid interference)
				radioShutdown();

				uint8_t tries;
				bool status = false;

				// Detect camera
				if(OV2640_isAvailable()) // OV2640 available
				{
					TRACE_INFO("IMG  > OV2640 found");

					if(config->ssdv_config.res == RES_MAX) // Attempt maximum resolution (limited by memory)
					{
						config->ssdv_config.res = RES_UXGA; // Try maximum resolution

						do {

							// Init camera
							OV2640_init(&config->ssdv_config);

							// Sample data from DCMI through DMA into RAM
							tries = 5; // Try 5 times at maximum
							do { // Try capturing image until capture successful
								status = OV2640_Snapshot2RAM();
							} while(!status && --tries);

							config->ssdv_config.res--; // Decrement resolution in next attempt (if status==false)

						} while(OV2640_BufferOverflow() && config->ssdv_config.res >= RES_QVGA);

						config->ssdv_config.res = RES_MAX; // Revert register

					} else { // Static resolution

						// Init camera
						OV2640_init(&config->ssdv_config);

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
					encode_ssdv(image, image_len, config, gimage_id-1);
				}

			} else {

				image_len = OV2640_getBuffer(&image);
				TRACE_INFO("IMG  > Image size: %d bytes", image_len);

				TRACE_INFO("IMG  > Camera disabled");
				TRACE_INFO("IMG  > Encode/Transmit SSDV ID=%d", gimage_id);
				encode_ssdv(image, image_len, config, gimage_id);

			}
		}

		time = waitForTrigger(time, &config->trigger);
	}
}

void start_image_thread(module_conf_t *conf)
{
	if(config->init_delay) chThdSleepMilliseconds(config->init_delay);
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

