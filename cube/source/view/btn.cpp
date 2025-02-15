/*
    Button processor
*/

#include "btn.h"
#include "../sys/stm32drv.h"
#include "../sys/maincfg.h"

#if HWVER < 2
#define BTN_PIN_UP      GPIOA, GPIO_PIN_2
#define BTN_PIN_SEL     GPIOA, GPIO_PIN_1
#define BTN_PIN_DN      GPIOA, GPIO_PIN_0
#else
#define BTN_PIN_UP      GPIOB, GPIO_PIN_9
#define BTN_PIN_SEL     GPIOB, GPIO_PIN_8
#define BTN_PIN_DN      GPIOB, GPIO_PIN_7
#endif // HWVER

#define BSTATE(btn)     HAL_GPIO_ReadPin(btn.gpiox, btn.pin)
#define PUSHED(state)   ((state) == GPIO_PIN_RESET)

namespace Btn {

    static bool     _sleep = false;         // в sleep-режиме в прерываниях мы не делаем обработку
                                            // на нажатия. Прерывание тут нужно только для пробуждения.
    static uint32_t _tickact = 0;           // кол-во тиков после крайней активности
    static hnd_t    _hold = NULL;           // обработчик зажатой кнопки
                                            // его нужно устанавливать при обычном onclick
                                            // при любом другом push/release он будет сброшен

    typedef struct {
        code_t          code;               // Код кнопки
        GPIO_TypeDef *  gpiox;              // группа пинов
        uint16_t        pin;                // пин
        GPIO_PinState   state;              // текущее состояние пина кнопки
        uint32_t        tmpsh, tmrls;       // время предыдущего нажатия на кнопку и отпускания (для антидребезга)
        uint8_t         cntsmpl, cntlong;   // счётчик сработавших событий simple и long
        bool            evtok;              // указывает на то, что hnd уже сработало при текущем удержании
        hnd_t           hndsmpl, hndlong;   // обработчики событий simple и long
    } el_t;

    static el_t btnall[3] = {
        { UP },
        { SEL },
        { DN }
    };

    static void _setpin(el_t &b, GPIO_TypeDef *gpiox, uint16_t pin) {
        b.gpiox = gpiox;
        b.pin = pin;
        b.state = BSTATE(b);
        b.tmrls = HAL_GetTick();
        if (PUSHED(b.state)) {
            // pushed
            b.tmpsh = b.tmrls;
            b.evtok = true;
        }
        else {
            b.tmpsh = 0;
            b.evtok = false;
        }
    }

    static void _read(el_t &b) {
        auto st = BSTATE(b);
        bool pushed = PUSHED(st);
        bool chg = st != b.state;
        auto tm = HAL_GetTick();

        if (chg) {
            _hold = NULL;
            _tickact = 0;
            if (pushed && ((tm-b.tmrls) > BTN_FILTER_TIME)) {
                // отфильтрованное нажатие
                b.tmpsh = tm;
                if (!b.evtok && (b.hndsmpl != NULL) && (b.hndlong == NULL)) {
                    // hndsmpl при отсутствующем hndlong
                    // срабатывает при нажатии
                    b.cntsmpl ++;
                    b.evtok = true;
                }
            }
            b.tmrls = tm;
            b.state = st;
        }
        else
        if (!pushed && (b.tmpsh > 0) && ((tm-b.tmrls) > BTN_FILTER_TIME)) {
            // отфильтрованное отпускание
            if (!b.evtok && (b.hndsmpl != NULL) && (b.hndlong != NULL)) {
                // hndsmpl при наличии hndlong
                // срабатывает при отпускании
                b.cntsmpl ++;
            }
            b.tmpsh = 0;
            b.evtok = false;
        }

        if (!b.evtok && (b.hndlong != NULL) && pushed && (b.tmpsh > 0) && ((tm-b.tmpsh) > BTN_LONG_TIME)) {
            b.cntlong ++;
            b.evtok = true;
        }
    }

    // ------------------------------------

    void init() {
        _sleep = false;
        _tickact = 0;
        flip180(cfg->flip180);
    }

    void set(code_t code, hnd_t hndsmpl, hnd_t hndlong) {
        for (auto &b: btnall)
            if (b.code == code) {
                b.hndsmpl = hndsmpl;
                b.hndlong = hndlong;
                return;
            }
    }

    void hold(hnd_t hnd) {
        if (!ispushed())
            return;
        _hold = hnd;
    }

    void sleep() {
        _sleep = true;
    }

    bool isactive(uint32_t tickcnt) {
        return _tickact <= tickcnt;
    }

    bool ispushed() {
        for (const auto &b: btnall)
            if (PUSHED(BSTATE(b)))
                return true;
        return false;
    }

    void flip180(bool flip) {
        if (cfg->flip180 != flip)
            (*cfg)->flip180 = flip;
        
        for (auto &b: btnall)
            switch (b.code) {
                case UP:
                    if (flip)
                        _setpin(b, BTN_PIN_DN);
                    else
                        _setpin(b, BTN_PIN_UP);
                    break;
                case SEL:
                        _setpin(b, BTN_PIN_SEL);
                    break;
                case DN:
                    if (flip)
                        _setpin(b, BTN_PIN_UP);
                    else
                        _setpin(b, BTN_PIN_DN);
                    break;
            }
    }

    void tick() {
        if (_hold != NULL)
            _hold();
        else
            _tickact ++;
        
        for (auto &b: btnall) {
            _read(b);

            if (b.hndsmpl != NULL)
                for (; b.cntsmpl > 0; b.cntsmpl--)
                    b.hndsmpl();
            else
            if (b.cntsmpl > 0)
                b.cntsmpl = 0;
            if (b.hndlong != NULL)
                for (; b.cntlong > 0; b.cntlong--)
                    b.hndlong();
            else
            if (b.cntlong > 0)
                b.cntlong = 0;
        }
    }

}; // namespace Btn

#ifdef __cplusplus
    extern "C"
#endif
void btn_byexti(uint16_t pin) {
    if (Btn::_sleep)
        return;
    for (auto &b: Btn::btnall)
        if ((b.gpiox != NULL) && (b.pin == pin))
            Btn::_read(b);
}
