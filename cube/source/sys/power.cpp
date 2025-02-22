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
#if HWVER < 2
    // SPI
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    // bmp280 cs
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    // display cs
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    // sdcard cs
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#else
    /*
    // SPI
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    // bmp280 cs
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    // display cs
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    // sdcard cs
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    // sdcard en
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_4|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif // HWVER
}

static void _spi_on() {
#if HWVER < 2
    // bmp280 cs
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    // display cs
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    // sdcard cs
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#else
    /*
    // bmp280 cs
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    // display cs
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    // sdcard cs
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    // sdcard en
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_4|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif // HWVER

    HAL_SPI_Init(&hspi1);
    HAL_Delay(20); // на f411 между SPI-init и сбором с bmp280 без этой паузы показания зависают
}

/* ------  power on/off  --------- */

//extern "C" void Error_Handler(void);
#if HWVER >= 2
extern ADC_HandleTypeDef hadc1;
#endif // HWVER

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
    Если шина hwen всегда включена - потребление 155-160 мкА для g473.
    Если эту шину подключать/отключать через транзистор, происходит много глюков на отключении.
    Если hwen завести напрямую на пин, то переключение этой шины в 0 даёт +10мА потребление,
    именно мА, а не мкА - так много, что это заметно даже при активном режиме (35-45 + 10 = 45-55 мА).
    А вместе с sd- картой - ещё +50мА. Видимо это связано с активностью SPI пинов, даже когда
    протокол SPI отключен.
    
    Если hwen пин переводить в режим GPIO_MODE_ANALOG, то потребление падает до 105-109 мкА,
    а SD-карта даёт +1мА. Всё-таки надо разводить питание SD-карты и дисплея по разным пинам.
    При этом, установка 0/1 на пине ничего не меняет, главное это делать ДО смены режима.

    Интересно, но получается, что снижение потребления произошло не из-за переключения режима,
    а из-за заведения питания дисплея на пин. Потому что, даже если не мненять режим, а
    состояние всегда оставлять 1, то низкое потребление примерно такое же, как и со сменой
    режима.
    А может быть эти 20-30мкА давал подтягивающий резистор 100к на затворе hwen, который в прошлых
    тестах (когда вместо транзистора была впаяна перемычка напрямую на +3.3в) ещё не был выпаян.

    Попытка так же перевести пины dspl_dc и dspl_rst в GPIO_MODE_ANALOG почему-то возвращают
    потребление обратно в 160мкА.

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
    Отключение spi/adc/usb перед уходом в shutdown ни на что не влияет.


    _____________________________________________
    Теперь отдельно о переходе на f411.

    Изначально большая проблема уменьшить энергопотребление ниже 0.8 мА (830мкА).
    Во-первых тут уже в любом случае надо отключать adc.

    Попробовал ту же плату (v2.0) на чистой прошивке которая сразу же уходит в сон.
    Без SPI и прочих модулей.
    Перевод всех пинов в INPUT-режим оставляет потребление 1.8мА.
    Перевод всех пинов в Analog снижает до 0.8 мА. И если при этом выпаять bmp280
    то потребление снижается до 60мкА - это вообще самый минимум, который удалось
    достичь, и всё равно это далеко до заявленных 10мкА.

    Осталось понять, эти 0.8 мА просто совпали или это действительно в обоих случаях
    кушает bmp280. В пустой прошивке нет возможности перевести bmp280 в сон.
    На полной прошивке опытным путём определил, если не уводить bmp в сон, это прибавляет
    примерно 0.6-0.8 мА к потреблению. А выпаивание bmp280 не даёт заметного уменьшения
    потребления по сравнениб, когда bmp280 есть и он тоже уходит в сон. Это значит,
    что на полной прошивке почему-то не отключаются некоторые службы, которые
    изначально не запускаются на полной прошивке.

    
*/

#include "usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceFS;

#if HWVER < 2
extern "C" void MX_USB_Device_Init(void);
#define usb_init()      MX_USB_Device_Init()
#else
extern "C" void MX_USB_DEVICE_Init(void);
#define usb_init()      MX_USB_DEVICE_Init()
#endif

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
#if HWVER >= 2
    HAL_ADC_DeInit(&hadc1);
#endif
    USBD_Stop(&hUsbDeviceFS);
    USBD_DeInit(&hUsbDeviceFS);

#if HWVER >= 2
    // Попробуем отключить все пины
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|
                            GPIO_PIN_9|GPIO_PIN_10||GPIO_PIN_11||GPIO_PIN_12|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|
                            GPIO_PIN_10|GPIO_PIN_10||GPIO_PIN_13||GPIO_PIN_14|GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif
}

#if HWVER >= 2
extern "C"
void GPIO_Init(void);
#endif

static void _on() {
#if HWVER >= 2
    // в _off мы отключаем все пины, поэтому проще их просто стандартно проинициировать
    GPIO_Init();

    HAL_ADC_Init(&hadc1);
#endif
    usb_init();

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
#if HWVER < 2
    void hwen(bool en) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, en ? GPIO_PIN_SET : GPIO_PIN_RESET);

        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = en ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
#endif // if HWVER < 2
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
            jmp::sleep();
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
            jmp::init();
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
