#include "ch.h"
#include "hal.h"
#include "debug.h"

const SerialConfig uart_config =
{
    115200,     // baud rate
    0,          // CR1 register
    0,          // CR2 register
    0           // CR3 register
};

mutex_t trace_mtx; // Used internal to synchronize multiple chprintf in debug.h

