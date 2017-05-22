#ifndef __IMG_H__
#define __IMG_H__

#include "ch.h"
#include "hal.h"

THD_FUNCTION(moduleIMG, arg);

extern mutex_t camera_mtx;

#endif

