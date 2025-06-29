
#include "log.h"

#ifdef USE_CONSOLE

#include "usbd_cdc_if.h"
#include "stm32drv.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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

typedef char buf_str_t[128];
#define BUFSZ 50
static buf_str_t _buf[BUFSZ];
static uint8_t _bufi = 0;
static void bufput(const char *s) {
    char *b = _buf[_bufi];
    strncpy(b, s, sizeof(buf_str_t));
    b[sizeof(buf_str_t)-1] = '\0';
}
static char* bufnxt() {
    _bufi++;
    _bufi %= BUFSZ;
    return _buf[_bufi];
}
void logbufinit() {
    for (int i = 0; i < BUFSZ; i++)
        _buf[i][0] = '\0';
}
const char * logbufbeg(logbuf_iterator_t *iter) {
    iter->i = _bufi+1;
    iter->i %= BUFSZ;
    iter->n = 0;
    
    const char *b = _buf[iter->i];
    for (int n=0; (*b == '\0') && (n < BUFSZ); n++) {
        iter->i ++;
        iter->i %= BUFSZ;
        b = _buf[iter->i];
    }
    
    return *b != '\0' ? b : NULL;
}
const char * logbufnxt(logbuf_iterator_t *iter) {
    if (iter->n+1 >= BUFSZ)
        return NULL;
    
    iter->i ++;
    iter->i %= BUFSZ;
    iter->n ++;

    const char *b = _buf[iter->i];
    return *b != '\0' ? b : NULL;
}

static void vtxtlog(const char *s, va_list ap) {
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;

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

    bufput(str);
    bufnxt()[0] = '\0';
    
    // при неподключенном usb tx может вызывать
    // USBD_BUSY, в результате мы будем ждать
    // несуществующую отправку.
    if (hcdc->TxState != 0)
        return;
    CDC_Transmit_FS((uint8_t *)str, len);
    // надо дождаться окончания передачи, т.к. она ассинхронная, а буфер str уничтожается при выходе
    for (volatile int i = 0; (i < 100000) && (hcdc->TxState != 0); i++);
}

void conslog(const char *s, ...) {
    va_list ap;
    char str[strlen(s)+1];

    strcpy(str, s);
    
    va_start (ap, s);
    vtxtlog(str, ap);
    va_end (ap);
}

#endif // USE_CONSOLE
