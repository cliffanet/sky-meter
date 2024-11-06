/*
    Power
*/

#ifndef _sys_power_H
#define _sys_power_H

#include "../../def.h"
#include <stdint.h>

#define PWR_SLEEP_TIMEOUT   30000

#ifdef __cplusplus
namespace pwr {
    
    void init();
};

 extern "C" {
#endif

void pwr_tick();

#ifdef __cplusplus
}
#endif


#endif // _sys_power_H
