/*
    Clock
*/

#ifndef _sys_clock_H
#define _sys_clock_H

#include "../../def.h"
#include <stdint.h>

/* ------------------------------------------------------------------------------------------- *
 *      операции с количеством тиков
 * ------------------------------------------------------------------------------------------- */
uint32_t utm();
uint32_t utm_diff(uint32_t prev, uint32_t &curr);
uint32_t utm_diff(uint32_t prev);

/* ------------------------------------------------------------------------------------------- *
 *      операции со временем в формате tm_t
 * ------------------------------------------------------------------------------------------- */
typedef struct __attribute__((__packed__)) tm_s {
	uint16_t year;     // Year                         (1999..2099)
	uint8_t  mon;      // Month                        (1..12)
	uint8_t  day;      // Day of month                 (1..31)
	uint8_t  h;        // Hour of day                  (0..23)
	uint8_t  m;        // Minute of hour               (0..59)
	uint8_t  s;        // Second of minute             (0..59)
    uint8_t  tick;     // tick count (1 tick = TIME_TICK_INTERVAL ms)
    
    uint8_t operator== (const struct tm_s & tm);
    bool operator!= (const struct tm_s & tm) { return !(*this == tm); };
} tm_t;

#define TIME_TICK_INTERVAL      10

/* ------------------------------------------------------------------------------------------- *
 *      часы
 * ------------------------------------------------------------------------------------------- */

tm_t tmNow();
bool tmAdjust(const tm_t &tm, uint8_t wday);

#endif // _sys_clock_H
