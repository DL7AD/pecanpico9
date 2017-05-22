/**
  * Flash driver
  * The driver code origin is
  * https://github.com/mabl/ARMCM3-STM32F107-BOOTLOADER/tree/master/ARMCM3-STM32F107-BOOTLOADER/flash
  * https://github.com/tegesoft/flash-stm32f407/tree/master/flash
  */

#include "helper.h"
#include "flash.h"
#include <ch.h>
#include <hal.h>
#include <ff.h>

void flashJumpApplication(uint32_t address)
{
    typedef void (*funcPtr)(void);

    uint32_t jumpAddr = *(uint32_t*)(address + 0x04); /* reset ptr in vector table */
    funcPtr usrMain = (funcPtr)jumpAddr;

    /* Reset all interrupts to default */
    chSysDisable();

    /* Clear pending interrupts just to be on the save side */
    SCB->ICSR = SCB_ICSR_PENDSVCLR_Pos;

    /* Disable all interrupts */
    int i;
    for(i = 0; i < 8; ++i)
        NVIC->ICER[i] = NVIC->IABR[i];

    /* Set stack pointer as in application's vector table */
    __set_MSP(*(uint32_t*)address);
    usrMain();
}
