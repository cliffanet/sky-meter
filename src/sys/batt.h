/*
    Batt
*/

#ifndef _sys_batt_H
#define _sys_batt_H

#include "../../def.h"
#include <stdint.h>


namespace batt {

    uint16_t raw();
    uint8_t val05();

    typedef enum {
        CHRG_OFF = 0,
        CHRG_STD,
        CHRG_HI
    } chrg_t;

    chrg_t charge();

} // namespace batt 

#endif // _sys_batt_H
