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

//#define PWRDEBUG

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

/* ------  register management  --------- */

// класс гашения перефирии
// при создании класса все нужные системные регистры сохраняются в нём и обнуляются
// при уничтожении класса регистры восстанавливаются
typedef struct {
    ADC_TypeDef             adc;
    SPI_TypeDef             spi;
    USB_OTG_GlobalTypeDef   usb;
} sysr_t;
static sysr_t _snull;

static inline sysr_t _sysr_get() {
    return { *ADC1, *SPI1, *USB_OTG_FS };
}
static inline void _sysr_set(sysr_t r) {
    *ADC1       = r.adc;
    *SPI1       = r.spi;
#ifndef PWRDEBUG
    *USB_OTG_FS = r.usb;
#endif
}

class _PStop {
private:
    sysr_t _ssave;

    class _gpio {
    public:
        uint32_t mode, type, speed, pupd;
        _gpio() : mode(0), type(0), speed(0), pupd(0) {}
        _gpio(GPIO_TypeDef *gpio) :
            mode    (gpio->MODER),
            type    (gpio->OTYPER),
            speed   (gpio->OSPEEDR),
            pupd    (gpio->PUPDR)
        {}
        void rest(GPIO_TypeDef *gpio) {
            gpio->PUPDR     = pupd;
            gpio->MODER     = mode;
            gpio->OSPEEDR   = speed;
            gpio->OTYPER    = type;
        }
    };
    _gpio _a, _b, _c;


    static void _gpio_set(GPIO_TypeDef *gpio, uint16_t mask = 0xffff, uint32_t mode = 0xffffffff, uint32_t type = 0, uint32_t speed = 0, uint32_t pupd = 0) {
        uint32_t msk2 = 0;
        uint32_t f1 = 1, f2 = 3;
        for (int n = 0; n < 32; n++, f1 <<= 1, f2 <<= 2)
            if ((mask & f1) > 0)
                msk2 |= f2;
        gpio->PUPDR     = (gpio->PUPDR & (~msk2))   | (pupd & msk2);
        gpio->MODER     = (gpio->MODER & (~msk2))   | (mode & msk2);
        gpio->OSPEEDR   = (gpio->OSPEEDR & (~msk2)) | (speed& msk2);
        gpio->OTYPER    = (gpio->OTYPER & (~mask))  | (type & mask);
    }
#define M(x)    (1 << x)

public:
    _PStop() :
        _ssave(_sysr_get()),
        _a(GPIOA),
        _b(GPIOB),
        _c(GPIOC)
    {
        while (Btn::ispushed())
            asm("");
        
        _tmr = 0;
    
    #ifdef USE_MENU
        Menu::clear();
    #endif // USE_MENU
        
        Dspl::off();
        Btn::sleep();

        // -------------------------
        _sysr_set(_snull);
        _gpio_set(GPIOA
#ifdef PWRDEBUG // не трогаем: usb-пины
                        , ~( M(11) | M(12) )
#endif
                );
        _gpio_set(GPIOB, ~( M(7) | M(8) | M(9) | M(10) )); // не трогаем: кнопки и bmp280-cs
        _gpio_set(GPIOC);
    }
    ~_PStop() {
        _a.rest(GPIOA);
        _b.rest(GPIOB);
        _c.rest(GPIOC);
        _sysr_set(_ssave);

        CONSOLE("init");
        pwr::init();
        Dspl::on();
        Btn::init();
        jmp::init();
    }

    // временное возобновление spi
    // через класс нельзя, т.к. возврат (сброс) надо делать не всегда (лучше вручную)
    void spion() {
        *SPI1 = _ssave.spi;
        _gpio_set(GPIOA, M(5) | M(6) | M(7),
            _a.mode, _a.type, _a.speed, _a.pupd
        );
    }
    void spioff() {
        *SPI1 = _snull.spi;
        _gpio_set(GPIOA, M(5) | M(6) | M(7));
    }
};

/* ------  power on/off  --------- */

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
    потребления по сравнению, когда bmp280 есть и он тоже уходит в сон. Это значит,
    что на полной прошивке почему-то не отключаются некоторые службы, которые
    изначально не запускаются на пустой прошивке.

    _____________________________________________
    Вторая попытка разобраться с f411.

    Взял отдельную dev-плату на f401ret, удалось выяснить:
        - очень много жрут пины, если их не переводить в нужные состояния
        - потребление сильно растёт, если не включать тактирование блока пинов (особенно GPIOC)
        - лучше всего снижается потребление при переключении пинов в analog (можно всем блоком)
        - альтернативно (по ситуации) - включение на вход с подтяжкой к "+" или "-"
        - если не делать подтяжку при режиме цифр-вход, состояние (триггер) прыгает и это сильно жрёт
        - на голой прошивке на плате с f401ret удалось снизить потр. до 130 мкА, ниже никак (только режимом пинов)
    
    После этого вернулся к плате sky-meter v.2 и выяснилось, что больше всего жрет USB,
    который почему-то не отключается - около 200+ мкА.
    Удалось снизить потребление до 120-130мкА суммарно за счёт:
        - перевода всех пинов в аналог (прям без разбора пачкой),
        - отключением USB через изменение регистра  USB_OTG_FS
            - запоминаем его значение до инициализации
            - при отключении питания переписываем весь блок регистров на значение "до инициализации"
    
    Ниже 125 мкА уменьшить пока никак не удаётся. Как получились 60 мкА, непонятно. Тогда выпаивался bmp280,
    но врядли он в спящем режиме 60+ мкА кушает.

    Вообще, хорошей практикой, наверное будет для ухода в сон запоминать все нужные регистры,
    полностью их сбрасывать и потом восстанавливать. Только оставить кнопки и CS-пины, подтянутые к верху.
    Это даже удобнее, чем тащить сюда и работать с HAL-структурами и функциями вроде HAL_SPI_DeInit и т.п.
    
*/

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
extern "C" {
#endif

void pwr_preinit() {
    // тут сохраняем все регистры сразу после старта:
    _snull = _sysr_get();;
}

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
        case PWR_OFF: {
            CONSOLE("power off");
            jmp::sleep();

            _PStop _s;

            bool slp = true;
            while (slp) {
#ifndef PWRDEBUG
                _tmr_stop();
                _stop();
#endif
                CONSOLE("power resume: _tmr=%d", _tmr);

                uint32_t beg = HAL_GetTick();
                while (Btn::ispushed())
                    if (slp && ((HAL_GetTick() - beg) > 2000)) {
                        CONSOLE("power on: _tmr=%d", _tmr);
                        slp = false;
                    }
            }
            return;
        }

        case PWR_SLEEP: {
            CONSOLE("sleep");
            _PStop _s;
            
            for (;;) {
#ifdef PWRDEBUG
                while (_tmr == 0) asm("");
#else
                _stop();
#endif
                CONSOLE("sleep resume: _tmr=%d", _tmr);

                if (_tmr == 0) {
                    // проснулись по кнопке, продолжаем инициализацию
                    jmp::sleep2gnd(); // для ускорения инициализации высотомера
                    // При уходе в сон мы сбрасываем буфер AltCalc и его _pressgnd.
                    // Но при этом остаётся работать AltSleep и поддерживает свой
                    // _pressgnd. При выходе из сна jmp::sleep2gnd() перекидывает
                    // _pressgnd из AltSleep в AltCalc, это ускоряет его инициализацию.
                    // Но делать это надо только при выходе по кнопке.
                    // Выход по jmp::sleep2toff(...) сбрасывает AltSleep и его
                    // _pressgnd становится уже неактуальным.
                    break;
                }

                _s.spion();

                // проверка высотомера, не на до ли просыпаться
                bool toff = jmp::sleep2toff(_tmr);
                _tmr = 0;
                if (toff)
                    // проснулись по началу подъёма
                    break;
                
                // продолжаем сон, но во сне всё равно обновляем состояние зарядки
                batt::tick(true);
                _s.spioff();
            }

            //CONSOLE("sleep end");
            while (Btn::ispushed())
                asm("");
            
            return;
        }
        
        case PWR_PASSIVE:
            _stop();
            return;
        
        default:
            while (_tmr == 0) asm("");
    }
}

#ifdef __cplusplus
}
#endif
