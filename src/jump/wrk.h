#ifndef _jump_wrk_H
#define _jump_wrk_H

#include <stdint.h>

// Шаг отображения высоты
#define ALT_STEP                5
// Порог перескока к следующему шагу
#define ALT_STEP_ROUND          3

// Интервал обнуления высоты (ms)
#define ALT_AUTOGND_INTERVAL    600000

namespace jmp {
    void init();
    void stop();
    void setdraw();
    void pagenxt();
    void resetgnd();
    void resetmode();
    uint8_t chipid();
    float press();
} // namespace jmp

#endif // _jump_wrk_H