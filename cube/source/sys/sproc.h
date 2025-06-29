/*
    Dynamic process manager
*/

#ifndef _sys_proc_H
#define _sys_proc_H

#include "../def.h"
#include <stdint.h>


#define SPRC_VAR uint16_t __sproc_line = 0;
#define SPRC_INIT __sproc_line = 0;
#define SPROC \
            switch (__sproc_line) { \
                case 0: {

#define SPRC_PNT __sproc_line = __LINE__; }                 case __LINE__: {
#define SPRC_BRK __sproc_line = __LINE__; return true; }    case __LINE__: {
#define SPRC_END \
                    break; \
                } \
                default: \
                    CONSOLE("Worker line fail: %d", __sproc_line); \
            }

namespace proc {
    typedef bool (*elem_t)();

    uint32_t add(elem_t proc, bool multi = true);
    bool isactive();
    bool isactive(uint32_t id);
    void run();
}


#endif // _sys_proc_H
