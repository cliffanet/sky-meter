/*
    Clock
*/

#include "stm32drv.h"
#include "clock.h"

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

uint16_t tmRand() {
    RTC_TimeTypeDef sTime = {0};
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        return 0;
    return sTime.SubSeconds * sTime.Seconds * sTime.Minutes * sTime.Hours;
}

bool tmSetFlag() {
    auto f = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
    return f == '*';
}

bool tmAdjust(const tm_t &tm, uint8_t wday) {
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours       = tm.h;
    sTime.Minutes     = tm.m;
    sTime.Seconds     = tm.s;
    sTime.SubSeconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        return false;

    RTC_DateTypeDef sDate = {0};
    sDate.WeekDay       = wday;
    sDate.Month         = tm.mon;
    sDate.Date          = tm.day;
    sDate.Year          = tm.year % 100;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        return false;
    
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, '*');

    return true;
}
