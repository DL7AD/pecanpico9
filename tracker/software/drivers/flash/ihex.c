/**
  * Flash driver
  * The driver code origin is
  * https://github.com/mabl/ARMCM3-STM32F107-BOOTLOADER/tree/master/ARMCM3-STM32F107-BOOTLOADER/flash
  * https://github.com/tegesoft/flash-stm32f407/tree/master/flash
  */

/*
 *  ihex.c
 *  Utility functions to create, read, write, and print Intel HEX8 binary records.
 *
 *  Written by Vanya A. Sergeev <vsergeev@gmail.com>
 *  Version 1.0.5 - February 2011
 *
 */

#include "ihex.h"

/* Initializes a new IHexRecord structure that the paramater ihexRecord points to with the passed
 * record type, 16-bit integer address, 8-bit data array, and size of 8-bit data array. */
int New_IHexRecord(int type, uint16_t address, const uint8_t *data, int dataLen,
    IHexRecord *ihexRecord) {
  /* Data length size check, assertion of ihexRecord pointer */
  if (dataLen < 0 || dataLen > IHEX_MAX_DATA_LEN / 2 || ihexRecord == NULL)
    return IHEX_ERROR_INVALID_ARGUMENTS;

  ihexRecord->type = type;
  ihexRecord->address = address;
  memcpy(ihexRecord->data, data, dataLen);
  ihexRecord->dataLen = dataLen;
  ihexRecord->checksum = Checksum_IHexRecord(ihexRecord);

  return IHEX_OK;
}

/* Utility function to calculate the checksum of an Intel HEX8 record */
uint8_t Checksum_IHexRecord(const IHexRecord *ihexRecord) {
  uint8_t checksum;
  int i;

  /* Add the data count, type, address, and data bytes together */
  checksum = ihexRecord->dataLen;
  checksum += ihexRecord->type;
  checksum += (uint8_t) ihexRecord->address;
  checksum += (uint8_t) ((ihexRecord->address & 0xFF00) >> 8);
  for (i = 0; i < ihexRecord->dataLen; i++)
    checksum += ihexRecord->data[i];

  /* Two's complement on checksum */
  checksum = ~checksum + 1;

  return checksum;
}

