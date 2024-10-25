/*
    Button processor
*/

#ifndef _view_btn_H
#define _view_btn_H

#include "../../def.h"
#include <stdint.h>
#include <stddef.h>


// минимальное время между изменениями состояния кнопки
#define BTN_FILTER_TIME     10
// время длинного нажатия
#define BTN_LONG_TIME       1000

#ifdef __cplusplus

namespace Btn {

    // Коды кнопок
    typedef enum {
        UP = 1,
        SEL,
        DN
    } code_t;

    typedef void (*hnd_t)();


    void init();
    void set(code_t code, hnd_t hndsmpl, hnd_t hndlong = NULL);

    void flip180(bool flip);

}; // namespace Btn

    extern "C"
#endif // __cplusplus
    void btn_byexti(uint16_t pin);

#endif // _view_btn_H
