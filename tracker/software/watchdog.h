#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include "types.h"

void register_thread_at_wdg(module_conf_t *thread_config);
void init_watchdog(void);

#endif

