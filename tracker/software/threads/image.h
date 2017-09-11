#ifndef __IMG_H__
#define __IMG_H__

#include "ch.h"
#include "hal.h"

void start_image_thread(module_conf_t *conf);
bool takePicture(ssdv_conf_t *conf, bool enableJpegValidation);
extern mutex_t camera_mtx;
extern uint8_t gimage_id;

#endif

