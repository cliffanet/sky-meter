/*
    STM32 main driver
*/

#ifndef _sys_stm32drv_H
#define _sys_stm32drv_H

#include "../def.h"

#if HWVER < 2
#include "stm32g4xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif

#endif // _sys_stm32drv_H
