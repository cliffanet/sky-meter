/*
    Error
*/

#ifndef _sys_err_H
#define _sys_err_H

#include "../def.h"
#include <stdint.h>
#include <stdlib.h>


namespace err {

    void add(uint8_t eno);
    void tostr(char *s, size_t sz);
    void tick();

} // namespace err 

#endif // _sys_err_H
