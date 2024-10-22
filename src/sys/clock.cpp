/*
    Clock
*/

#include "stm32drv.h"
#include "clock.h"

/* ------------------------------------------------------------------------------------------- *
 *      операции с количеством тиков
 * ------------------------------------------------------------------------------------------- */
uint32_t utm() {
    return HAL_GetTick();
}

uint32_t utm_diff(uint32_t prev, uint32_t &curr) {
    curr = utm();
    return curr - prev;
}

uint32_t utm_diff(uint32_t prev) {
    return utm() - prev;
}

/* ------------------------------------------------------------------------------------------- *
 *      операции со временем в формате tm_t
 * ------------------------------------------------------------------------------------------- */

uint8_t tm_s::operator==(const tm_s &tm) {
    return
        (this == &tm) ||
        (
            (this->year == tm.year) && (this->mon == tm.mon) && (this->day == tm.day) &&
            (this->h == tm.h) && (this->m == tm.m) && (this->s == tm.s)
        );
}

int32_t tm_s::operator-(const tm_s &tm) {
    /*
    DateTime
        dtbeg(tmbeg.year, tmbeg.mon, tmbeg.day, tmbeg.h, tmbeg.m, tmbeg.s),
        dtend(tmend.year, tmend.mon, tmend.day, tmend.h, tmend.m, tmend.s);
    
    int32_t tint = (dtend.unixtime() - dtbeg.unixtime()) * 1000;
    int8_t tick1 = tmend.tick - tmbeg.tick;
    int16_t tick = tick1;
    return tint + (tick * TIME_TICK_INTERVAL);
    */
    return 0;
}

tm_s tm_s::operator-(uint32_t ms) {
    return tm_s();
}

/* ------------------------------------------------------------------------------------------- *
 *      часы
 * ------------------------------------------------------------------------------------------- */

/*
tm_t tmNow(uint32_t earlerms) {
    if (earlerms == 0)
        return tmNow();
    
    auto tm = tmNow();
    
    DateTime dt1(tm.year, tm.mon, tm.day, tm.h, tm.m, tm.s);
    uint32_t ut = dt1.unixtime() - (earlerms / 1000);
    
    int8_t tick = (earlerms % 1000) / TIME_TICK_INTERVAL;
    tick = tm.tick - tick;
    
    ut += tick * TIME_TICK_INTERVAL / 1000;
    tick = tick % (1000 / TIME_TICK_INTERVAL);
    if (tick < 0) {
        ut --;
        tick += 100;
    }
    
    DateTime dt(ut);
    return { 
        year: dt.year(),
        mon : dt.month(),
        day : dt.day(),
        h   : dt.hour(),
        m   : dt.minute(),
        s   : dt.second(),
        tick: static_cast<uint8_t>(tick >= 0 ? tick : 0)
    };
}
*/

/* ------------------------------------------------------------------------------------------- *
 *  работа с часами
 * ------------------------------------------------------------------------------------------- */
extern RTC_HandleTypeDef hrtc;

tm_t tmNow() {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (
            (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) ||
            (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        )
        return { 0 };
    
    tm_t tm = {
        .year   = static_cast<uint16_t>(static_cast<uint16_t>(sDate.Year) + 2000),
        .mon    = sDate.Month,
        .day    = sDate.Date,
        .h      = sTime.Hours,
        .m      = sTime.Minutes,
        .s      = sTime.Seconds,
        .tick   = static_cast<uint8_t>(1000 / TIME_TICK_INTERVAL * (1023 - sTime.SubSeconds) / 1024),
    };

    return tm;
}

bool tmAdjust(const tm_t &tm, uint8_t wday) {
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours       = tm.h;
    sTime.Minutes     = tm.m;
    sTime.Seconds     = tm.s;
    sTime.SubSeconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
        return false;

    RTC_DateTypeDef sDate = {0};
    sDate.WeekDay       = wday;
    sDate.Month         = tm.mon;
    sDate.Date          = tm.day;
    sDate.Year          = tm.year % 100;

    return HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK;
}
