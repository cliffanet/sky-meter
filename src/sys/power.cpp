/*
    Power
*/

#include "stm32drv.h"
#include "power.h"
#include "batt.h"
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
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
}

static void _tmr_set(uint32_t ms) {
    _tmr_stop();
    // div16 делит частоту 32.768 кГц на 16,
    // получаем 2048 тик в секунду,
    // длительность 1 ms = 2048/1000
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, ms*2048/1000-1, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}

/* ------  spi on/off  --------- */
// нам надо отдельно вкл/выкл spi во сне,
// чтобы проверять барометр

extern SPI_HandleTypeDef hspi1;

static void _spi_off() {
    // spi даёт 0.22 mA
    HAL_SPI_DeInit(&hspi1);
    // Почему-то, до отключения spi подключение/отключение
    // дисплея (весь шлейф) добавляло около 0.1 мА.
    // Но после отключения spi отключение шлейфа дисплея
    // наоборот увеличивает потребление с 0.2 мА до 0.5 мА
}

static void _spi_on() {
    HAL_SPI_Init(&hspi1);
}

/* ------  power on/off  --------- */

//extern "C" void Error_Handler(void);
//extern ADC_HandleTypeDef hadc1;

//#include "usbd_cdc_if.h"
//extern USBD_HandleTypeDef hUsbDeviceFS;

// Замеры:
// В passive режиме цикл работы: 7 мс из 100
// В sleep режиме: 10 мс из 5000.
// Для sleep режима добавляется время вкл/выкл spi

// По потреблению
/*
    - существенно потребление снижает отключение spi (примерно -250 мкА)
    - отключение adc снижает на 10-15 мкА
    - отключение usb так же снижает на 10-15 мкА

    Если отключать только spi, потребление ~ 150-160 мкА (это g473, для g431 - 125-130 мкА).
    Отключение usb/adc даст ещё -30мкА (потребление ~ 125-130 мкА), но
    там уже небольшой гемор обратно инициировать.

    Попытка перед уходом в сон положить пины дисплея на землю только добавляют 30-50 мкА
    потребления, т.е. делают хуже.

    _____________________________________________
    Ещё пока по непонятной причине, любое взаимодействие по jtag (даже если просто
    перезагрузить девайс) что-то включает, и потребление увеличивается на 1.5 мА
    во всех режимах, в т.ч. и во сне.
    И сбросить это не помогает ничего - ни уход в сон, ни перезагрузка.
    Только передёргивание аккума помогает.

    _____________________________________________
    Потребление в активе: 30-32 мА.

    sd-карта потребляет 1-2 мА.
    _____________________________________________
    Переход в shutdown режим снижает потребление до 80мкА.
    Отключение spi/adc/usb перед уходом в shitdown ни на что не влияет.
*/

static void _off() {
    while (Btn::ispushed())
        asm("");
    
    _tmr = 0;

#ifdef USE_MENU
    Menu::clear();
#endif // USE_MENU
    
    Dspl::off();
    Btn::sleep();


    // отключение usb и adc даёт не более 20 мкА
    //HAL_ADC_DeInit(&hadc1);
    //USBD_Stop(&hUsbDeviceFS);
    //USBD_DeInit(&hUsbDeviceFS);
}

static void _on() {
    //HAL_ADC_Init(&hadc1);

    CONSOLE("init");
    pwr::init();
    Dspl::on();
    Btn::init();
    jmp::init();
}

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

#ifdef USE_MENU
    if (Menu::isactive())
        return PWR_ACTIVE;
#endif // USE_MENU

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
        batt::tick();

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
                CONSOLE("sleep beg");
                jmp::sleep();
                _tmr_set(2000);
                return;
            }
        }
    }
    
    switch (_m) {
        case PWR_OFF:
            CONSOLE("power off");
            _off();
            _spi_off();
            _tmr_stop();

            _stop();

            //HAL_ADC_DeInit(&hadc1);
            //USBD_Stop(&hUsbDeviceFS);
            //USBD_DeInit(&hUsbDeviceFS);
            //HAL_SuspendTick();
            //HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            //HAL_PWREx_EnterSHUTDOWNMode();
            CONSOLE("power resume: _tmr=%d", _tmr);

            {
                uint32_t beg = HAL_GetTick();
                bool ok = false;
                while (Btn::ispushed())
                    if (!ok && ((HAL_GetTick() - beg) > 2000)) {
                        CONSOLE("power on: _tmr=%d", _tmr);
                        ok = true;
                    }
            
                if (!ok)
                    return;
            }
            
            _spi_on();
            _on();
            return;

        case PWR_SLEEP:
            CONSOLE("sleep");
            _off();
            _spi_off();

            _stop();
            CONSOLE("sleep resume: _tmr=%d", _tmr);
            
            _spi_on();

            if (_tmr > 0) {
                // проверка высотомера, не на до ли просыпаться
                bool toff = jmp::sleep2toff(_tmr);
                _tmr = 0;
                if (!toff) {
                    batt::tick(true);
                    return;
                }
            }

            //CONSOLE("sleep end");
            while (Btn::ispushed())
                asm("");
            _on();
            return;
        
        case PWR_PASSIVE:
            _stop();
            return;
        
        default:
            while (_tmr == 0) asm("");
    }
}
