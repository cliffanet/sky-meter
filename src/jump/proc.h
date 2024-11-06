#ifndef _jump_proc_H
#define _jump_proc_H

#include <stdint.h>

// Шаг отображения высоты
#define ALT_STEP                5
// Порог перескока к следующему шагу
#define ALT_STEP_ROUND          3

// Интервал обнуления высоты (ms)
#define ALT_AUTOGND_INTERVAL    600000

namespace jmp {
    void init();
    void setdraw();
    void pagenxt();
    void resetgnd();
    void resetmode();
    uint8_t chipid();
    float press();
    bool isgnd();
    void tick(uint32_t ms);
} // namespace jmp

#endif // _jump_proc_H
