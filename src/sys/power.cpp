/*
    Power
*/

#include "stm32drv.h"
#include "power.h"
#include "maincfg.h"
#include "log.h"
#include "../jump/wrk.h"
#include "../view/btn.h"
#include "../view/menu.h"

typedef enum {
    PWR_OFF = 0,
    PWR_SLEEP,
    PWR_PASSIVE,
    PWR_ACTIVE
} pwr_mode_t;

static pwr_mode_t _m = PWR_ACTIVE;

static pwr_mode_t _mode() {
    if (!jmp::isgnd() && !cfg->lopwronjmp)
        return PWR_ACTIVE;
    if (!jmp::isgnd())
        return PWR_PASSIVE;
    if (Menu::isactive())
        return PWR_PASSIVE;
    if (Btn::isactive())
        return PWR_PASSIVE;
    
    return PWR_SLEEP;
}

static void _chg(pwr_mode_t m) {
    CONSOLE("[pwr change] %d => %d", _m, m);
    _m = m;
}

namespace pwr {
    
    void init() {

    }

};

/*
extern RTC_HandleTypeDef hrtc;

#ifdef __cplusplus
 extern "C" {
#endif

void SystemClock_Config(void);

#include "usbd_cdc_if.h"
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
    SystemClock_Config();
	char s[32];
    sprintf(s, "WAKEUP: %d\n", HAL_GetTick());
	CDC_Transmit_FS((uint8_t *)s, strlen(s));
    //CONSOLE("resuming: %d", HAL_GetTick());
}

#ifdef __cplusplus
 }
#endif
*/
#ifdef __cplusplus
 extern "C"
#endif
void pwr_delay(uint32_t ms) {
    auto m = _mode();
    if (_m != m)
        _chg(m);
    
    /*
    switch (m) {
        case PWR_OFF:
            return;
        case PWR_SLEEP:
            return;
        case PWR_PASSIVE:
            // div16 делит частоту 32.768 кГц на 16,
            // получаем 2048 тик в секунду,
            // длительность 1 ms = 2048/1000
            CONSOLE("pause: %d = 0x%04x; cur: %d", ms, ms*2048/1000, HAL_GetTick());
            HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
            HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2048, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
            HAL_SuspendTick();
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            HAL_ResumeTick();
            HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

            CONSOLE("resume: %d", HAL_GetTick());
            return;
        case PWR_ACTIVE:
            if (ms > 2)
                HAL_Delay(ms-1);
            return;
    }
    */
            if (ms > 2)
                HAL_Delay(ms-1);
}
