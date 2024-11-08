/*
    Button processor
*/

#include "btn.h"
#include "../sys/stm32drv.h"
#include "../sys/maincfg.h"


#define BTN_PIN_UP      GPIOA, GPIO_PIN_2
#define BTN_PIN_SEL     GPIOA, GPIO_PIN_1
#define BTN_PIN_DN      GPIOA, GPIO_PIN_0

#define BSTATE(btn)     HAL_GPIO_ReadPin(btn.gpiox, btn.pin)
#define PUSHED(state)   ((state) == GPIO_PIN_RESET)

namespace Btn {

    typedef struct {
        code_t          code;               // Код кнопки
        GPIO_TypeDef *  gpiox;              // группа пинов
        uint16_t        pin;                // пин
        GPIO_PinState   state;              // текущее состояние пина кнопки
        uint32_t        tmpsh, tmrls;       // время предыдущего нажатия на кнопку и отпускания (для антидребезга)
        uint8_t         cntsmpl, cntlong;   // счётчик сработавших событий simple и long
        hnd_t           hndsmpl, hndlong;   // обработчики событий simple и long
        bool            evtlong;            // указывает на то, что hndlong уже сработало при текущем удержании
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
            b.evtlong = true;
        }
        else {
            b.tmpsh = 0;
            b.evtlong = false;
        }
    }

    static void _read(el_t &b) {
        auto st = BSTATE(b);
        bool pushed = PUSHED(st);
        bool chg = st != b.state;
        auto tm = HAL_GetTick();

        if (chg) {
            if (pushed && ((tm-b.tmrls) > BTN_FILTER_TIME)) {
                // отфильтрованное нажатие
                b.tmpsh = tm;
                if ((b.hndsmpl != NULL) && (b.hndlong == NULL))
                    // hndsmpl при отсутствующем hndlong
                    // срабатывает при нажатии
                    b.cntsmpl ++;
            }
            b.tmrls = tm;
            b.state = st;
        }
        else
        if (!pushed && (b.tmpsh > 0) && ((tm-b.tmrls) > BTN_FILTER_TIME)) {
            // отфильтрованное отпускание
            if (!b.evtlong && (b.hndsmpl != NULL) && (b.hndlong != NULL))
                // hndsmpl при наличии hndlong
                // срабатывает при отпускании
                b.cntsmpl ++;
            b.tmpsh = 0;
            b.evtlong = false;
        }

        if (!b.evtlong && (b.hndlong != NULL) && pushed && (b.tmpsh > 0) && ((tm-b.tmpsh) > BTN_LONG_TIME)) {
            b.evtlong = true;
            b.cntlong ++;
        }
    }

    // ------------------------------------

    void init() {
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

    bool isactive(uint32_t ms) {
        auto tm = HAL_GetTick();
        for (const auto &b: btnall)
            if ((tm-b.tmrls) <= ms)
                return true;

        return false;
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
    for (auto &b: Btn::btnall)
        if ((b.gpiox != NULL) && (b.pin == pin))
            Btn::_read(b);
}
