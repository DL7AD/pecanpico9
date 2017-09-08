/**
  * This is the OV2640 driver
  */

#ifndef __OV5640_H__
#define __OV5640_H__

#include "ch.h"
#include "hal.h"
#include "types.h"

#define OV5640_USE_DMA_DBM  TRUE

bool OV5640_Snapshot2RAM(bool enableJpegValidation);
bool OV5640_Capture(void);
void OV5640_InitGPIO(void);
bool OV5640_BufferOverflow(void);
void OV5640_TransmitConfig(void);
void OV5640_init(ssdv_conf_t *config);
void OV5640_deinit(void);
bool OV5640_isAvailable(void);
uint32_t OV5640_getLightIntensity(void);

#endif
