/*
    Power
*/

#include "stm32drv.h"
#include "power.h"
#include "maincfg.h"
#include "log.h"
#include "../jump/proc.h"
#include "../view/btn.h"
#include "../view/dspl.h"
#include "../view/menu.h"

/* ------  RTC timer  --------- */
extern RTC_HandleTypeDef hrtc;
extern "C" void SystemClock_Config(void);

static __IO uint32_t _tmr = 0;

extern "C"
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
    _tmr += (HAL_RTCEx_GetWakeUpTimer(hrtc)+1) * 1000 / 2048;
}

static void _tmr_stop() {
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
}

static void _tmr_set(uint32_t ms) {
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    // div16 делит частоту 32.768 кГц на 16,
    // получаем 2048 тик в секунду,
    // длительность 1 ms = 2048/1000
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, ms*2048/1000-1, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}

/* ------  power mode  --------- */

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
    if (Menu::isactive())
        return PWR_ACTIVE;
    if (Btn::isactive(BTN_ACTIVE_SHORT))
        // в обработчике кнопок мы используем HAL_GetTick,
        // а он не работает при уходе в сон, интервалы считаются неверно,
        // поэтому надо обязательно выходить в active-mode
        // сразу же после любого нажатия
        return PWR_ACTIVE;
    
    if (!jmp::isgnd())
        return PWR_PASSIVE;
    if (Btn::isactive(BTN_ACTIVE_LONG))
        return PWR_PASSIVE;
    
    return PWR_SLEEP;
}

static void _stop() {
    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    HAL_ResumeTick();
    SystemClock_Config();
}

static void _sleep_beg() {
    CONSOLE("sleep beg");
    Dspl::off();
    jmp::sleep();
    _tmr_set(5000);
    Btn::sleep();
}

static void _sleep_tick() {
    CONSOLE("sleep tick");
    while (Btn::ispushed())
        asm("");
    
    _tmr = 0;
    
    _stop();
    CONSOLE("sleep resume: _tmr=%d", _tmr);
    
    if (_tmr > 0) {
        // проверка высотомера, не на до ли просыпаться
        bool toff = jmp::sleep2toff(_tmr);
        _tmr = 0;
        if (!toff)
            return;
    }

    CONSOLE("sleep end");
    while (Btn::ispushed())
        asm("");
    pwr::init();
    Dspl::on();
    Btn::init();
}

static void _poweroff() {
    CONSOLE("power off");
    while (Btn::ispushed())
        asm("");
    
    _tmr = 0;
    Menu::clear();
    Dspl::off();
    jmp::sleep();
    _tmr_stop();
    
    _stop();
    CONSOLE("power resume: _tmr=%d", _tmr);

    uint32_t beg = HAL_GetTick();
    bool ok = false;
    while (Btn::ispushed())
        if (!ok && ((HAL_GetTick() - beg) > 2000)) {
            CONSOLE("power on: _tmr=%d", _tmr);
            ok = true;
        }
    
    if (!ok)
        return;

    CONSOLE("init");
    pwr::init();
    Dspl::on();
    Btn::init();
    jmp::init();
}

/* ------  -------  --------- */

namespace pwr {
    
    void init() {
        _m = PWR_ACTIVE;
        _tmr_set(100);
    }

    void off() {
        _m = PWR_OFF;
    }
    void hwen(bool en) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, en ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
};

/* ------  -------  --------- */

#ifdef __cplusplus
 extern "C"
#endif
void pwr_tick() {
    if (_tmr > 0) {
        jmp::tick(_tmr);
        Btn::tick();
        Dspl::tick();

        _tmr = 0;

        // Надо забыть об использовании HAL_GetTick в качестве опорного времени
        // в длинных процессах. Дело в том, что он основан на таймере, который
        // мы отключаем, когда уходим в сон. Единственное, что мы можем использовать
        // как время-задающую цепочку - это RTC-Timer. Он, вроде б, один,
        // и использовать его нужно экономно - для любых отсчётов времени,
        // а так же - для пробуждения из сна.

        // Помимо этого надо помнить, что из сна мы можем выйти и до наступления
        // таймера, поэтому, например, нельзя бездумно вызывать jmp::tick(100);
        // каждый раз при выходе из сна, т.к. тут может оказаться интервал менее 100мс.
        // Нужно параллельно использовать отдельный счётчик, чисто под jmp::tick
    }

    if (_m > PWR_OFF) {
        auto m = _mode();
        if (_m != m) {
            CONSOLE("[pwr change] %d => %d", _m, m);
            _m = m;

            if (_m == PWR_SLEEP) {
                _sleep_beg();
                return;
            }
        }
    }
    
    switch (_m) {
        case PWR_OFF:
            _poweroff();
            return;
        case PWR_SLEEP:
            _sleep_tick();
            return;
        case PWR_PASSIVE:
            _stop();
            return;
        
        default:
            while (_tmr == 0) asm("");
    }
}
