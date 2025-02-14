/*
    Power
*/

#ifndef _sys_power_H
#define _sys_power_H

#include "../def.h"
#include <stdint.h>

#define LED_PIN_RED         GPIOA, GPIO_PIN_3
#define LED_PIN_BLUE        GPIOA, GPIO_PIN_4

#ifdef __cplusplus
namespace pwr {
    
    void init();
    void off();
    void hwen(bool en);
};

 extern "C" {
#endif

void pwr_tick();

#ifdef __cplusplus
}
#endif


#endif // _sys_power_H
