/**
  * Flash driver
  * The driver code origin is
  * https://github.com/mabl/ARMCM3-STM32F107-BOOTLOADER/tree/master/ARMCM3-STM32F107-BOOTLOADER/flash
  * https://github.com/tegesoft/flash-stm32f407/tree/master/flash
  */

#ifndef HELPER_H
#define HELPER_H

#include <stdint.h>

#define BOOTLOADER_SUCCESS           0
#define BOOTLOADER_ERROR_NOCARD      1
#define BOOTLOADER_ERROR_BADFS       2
#define BOOTLOADER_ERROR_NOFILE      3
#define BOOTLOADER_ERROR_READFAILURE 4
#define BOOTLOADER_ERROR_BADHEX      5
#define BOOTLOADER_ERROR_BADFLASH    6

/**
 * @brief Jump to application located in flash.
 * @param address Address to jump to.
 *
 * @author Code stolen from "matis"
 * @link http://forum.chibios.org/phpbb/viewtopic.php?f=2&t=338
 */
void flashJumpApplication(uint32_t address);

#endif /* HELPER_H */
