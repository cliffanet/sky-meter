/*
    Dynamic process manager
*/

#ifndef _sys_proc_H
#define _sys_proc_H

#include "../def.h"
#include <stdint.h>


namespace proc {
    typedef bool (*elem_t)();

    void add(elem_t proc);
    bool isactive();
    void run();
}


#endif // _sys_proc_H
