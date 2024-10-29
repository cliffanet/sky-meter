
#include "log.h"

#ifdef FWVER_DEBUG

//#include "navi/proc.h"
//#include "clock.h"
#include "usbd_cdc_if.h"

#include <stdio.h>
#include <stdarg.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

/* ------------------------------------------------------------------------------------------- *
 *  Логирование в консоль
 * ------------------------------------------------------------------------------------------- */
const char * __extrfname(const char * path) {
    int l = strlen(path);

    while (l > 0) {
        l--;
        if ((path[l] == '/') || (path[l] == '\\'))
            return path + l + 1;
    }

    return path;
}

static void vtxtlog(const char *s, va_list ap) {
    int len = vsnprintf(NULL, 0, s, ap), sbeg = 0;
    char str[len+60]; // +48=dt +12=debug mill/tick
    
    extern RTC_HandleTypeDef hrtc;
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    if (
            (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) &&
            (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK)
        )
        sbeg =
            snprintf(
                str, 32, "%2d.%02d.%04d %2d:%02d:%02d.%03ld ",
                sDate.Date, sDate.Month, sDate.Year,
                sTime.Hours, sTime.Minutes, sTime.Seconds,
                1000 * (sTime.SecondFraction-sTime.SubSeconds) / sTime.SecondFraction
            );
    
    len = vsnprintf(str+sbeg, len+1, s, ap) + sbeg;
    str[len] = '\n';
    len++;
    str[len] = '\0';
    
    CDC_Transmit_FS((uint8_t *)str, len);
    // надо дождаться окончания передачи, т.к. она ассинхронная, а буфер str уничтожается при выходе
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
    for (int i = 0; (i < 100) && (hcdc->TxState != 0); i++)
        HAL_Delay(2);
}

void conslog(const char *s, ...) {
    va_list ap;
    char str[strlen(s)+1];

    strcpy(str, s);
    
    va_start (ap, s);
    vtxtlog(str, ap);
    va_end (ap);
}

#endif // FWVER_DEBUG
