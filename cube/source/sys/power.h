/*
    Power
*/

#ifndef _sys_power_H
#define _sys_power_H

#include "../def.h"
#include <stdint.h>

#if HWVER < 2
#define LED_PIN_RED         GPIOA, GPIO_PIN_3
#define LED_PIN_BLUE        GPIOA, GPIO_PIN_4
#else
#define LED_PIN_RED         GPIOA, GPIO_PIN_0
#define LED_PIN_BLUE        GPIOA, GPIO_PIN_1
#endif // HWVER

#ifdef __cplusplus
namespace pwr {
    
    void init();
    void off();
#if HWVER < 2
    void hwen(bool en);
#endif
};

 extern "C" {
#endif

void pwr_tick();

#ifdef __cplusplus
}
#endif


#endif // _sys_power_H
