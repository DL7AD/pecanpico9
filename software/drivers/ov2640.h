/**
  * This is the OV2640 driver
  */

#ifndef __OV2640_H__
#define __OV2640_H__

#include "ch.h"
#include "hal.h"
#include "types.h"

bool OV2640_Snapshot2RAM(void);
void OV2640_InitDMA(void);
void OV2640_DeinitDMA(void);
void OV2640_InitDCMI(void);
void OV2640_CaptureDCMI(void);
void OV2640_DeinitDCMI(void);
void OV2640_InitGPIO(void);
uint32_t OV2640_getBuffer(uint8_t** buffer);
bool OV2640_BufferOverflow(void);
void OV2640_TransmitConfig(void);
void OV2640_init(ssdv_config_t *config);
void OV2640_deinit(void);
bool OV2640_isAvailable(void);

#endif
