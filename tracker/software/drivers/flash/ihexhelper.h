/**
  * Flash driver
  * The driver code origin is
  * https://github.com/mabl/ARMCM3-STM32F107-BOOTLOADER/tree/master/ARMCM3-STM32F107-BOOTLOADER/flash
  * https://github.com/tegesoft/flash-stm32f407/tree/master/flash
  */

#ifndef IHEXHELPER_H
#define IHEXHELPER_H

#include "helper.h"
#include <ff.h>
#include <stdint.h>

/**
 * @brief Flash the provided IHex @p file.
 * @param file IHex file describing the data to be flashed.
 * @return BOOTLOADER_SUCCESS on success
 * @return BOOTLOADER_ERROR_BADFLASH if an error occured when manipulating the flash memory
 * @return BOOTLOADER_ERROR_BADHEX if an error occured when reading the iHex file
 */
int flashIHexFile(FIL* file);

#endif /* IHEXHELPER_H */
