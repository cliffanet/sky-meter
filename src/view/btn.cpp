/*
    Button processor
*/

#include "btn.h"
#include "../sys/stm32drv.h"
#include "../sys/worker.h"
#include "../sys/maincfg.h"


#define BTN_PIN_UP      GPIOA, GPIO_PIN_2
#define BTN_PIN_SEL     GPIOA, GPIO_PIN_1
#define BTN_PIN_DN      GPIOA, GPIO_PIN_0

namespace Btn {

    class _wrkBtn : public Wrk {
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

        el_t btnall[3] = {
            { UP },
            { SEL },
            { DN }
        };

        void _setpin(el_t &b, GPIO_TypeDef *gpiox, uint16_t pin) {
            b.gpiox = gpiox;
            b.pin = pin;
        }

        void _read(el_t &b) {
            auto st = HAL_GPIO_ReadPin(b.gpiox, b.pin);
            bool pushed = st == GPIO_PIN_RESET;
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

    public:
        void set(code_t code, hnd_t hndsmpl, hnd_t hndlong) {
            for (auto &b: btnall)
                if (b.code == code) {
                    b.hndsmpl = hndsmpl;
                    b.hndlong = hndlong;
                    return;
                }
        }

        void byexti(uint16_t pin) {
            for (auto &b: btnall)
                if ((b.gpiox != NULL) && (b.pin == pin))
                    _read(b);
        }

        void flip180(bool flip) {
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

        state_t run() {
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

            return DLY;
        }
    };

    static _wrkBtn *_w = NULL;

    void init() {
        if (_w != NULL)
            return;
        
        _w = new _wrkBtn();
        if (_w == NULL)
            return;
        _w->flip180((*cfg)->flip180);
    }

    void set(code_t code, hnd_t hndsmpl, hnd_t hndlong) {
        if (_w != NULL)
            _w->set(code, hndsmpl, hndlong);
    }

    void flip180(bool flip) {
        (*cfg)->flip180 = flip;
        if (_w != NULL)
            _w->flip180(flip);
    }

}; // namespace Btn

#ifdef __cplusplus
    extern "C"
#endif
void btn_byexti(uint16_t pin) {
    if (Btn::_w != NULL)
        Btn::_w->byexti(pin);
}
