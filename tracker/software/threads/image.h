#ifndef __IMG_H__
#define __IMG_H__

#include "ch.h"
#include "hal.h"

#define IMG_ID_FLASH_ADDR		0x80A0000	/* Image ID flash memory address */
#define IMG_ID_FLASH_SIZE		(128*1024)	/* Image ID flash memory size */

extern const uint8_t noCameraFound[4071];

void start_image_thread(module_conf_t *conf);
bool takePicture(ssdv_conf_t *conf, bool enableJpegValidation);
extern mutex_t camera_mtx;

#endif

