#ifndef _jump_proc_H
#define _jump_proc_H

#include <stdint.h>
#include "logbook.h"
#include "ringsimple.h"

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
    float temp();
    bool isgnd();
    const LogBook::item_t &last();


#ifdef USE_JMPTRACE

#if defined(FWVER_DEBUG)
    #define JMP_TRACE_SEC       (12*60)
#else
    #define JMP_TRACE_SEC       (6*60)
#endif

    typedef struct {
        int alt;
        char mclc;
        char mchg;
    } log_t;
    typedef ring<log_t, JMP_TRACE_SEC * 10> log_ring_t;
    const log_ring_t & trace();
#endif // USE_JMPTRACE

    void sleep();
    bool sleep2toff(uint32_t ms);
    void sleep2gnd();

    void tick(uint32_t ms);
} // namespace jmp

#endif // _jump_proc_H
