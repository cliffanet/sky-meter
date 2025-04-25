
#include "cmdtxt.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

CmdTxt::CmdTxt(const char *id, const str_t *arg, uint8_t acnt) :
    id(id),
    _arg(arg),
    _acnt(acnt),
    _argc(0),
    _e("")
{}

const char *CmdTxt::argfetch() {
    if (argavail()) {
        auto a = _arg[_argc];
        _argc++;
        return a;
    }

    err("arg[%d] not exists", _argc+1);
    return NULL;
}

#define _arg(n)     auto n = argfetch(); if (n == NULL) return 0;

uint32_t CmdTxt::argnum() {
    _arg(s);
    uint32_t r = 0;

    for (int i = 1; *s != 0; s++, i++)
        if ((*s >= '0') && (*s <= '9'))
            r = r * 10 + (*s - '0');
        else {
            err("arg[%d, symb:%d] not number", _argc, i);
            break;
        }

    return r;
}

uint32_t CmdTxt::arghex() {
    _arg(s);
    uint32_t r = 0;

    for (int i = 1; *s != 0; s++, i++)
        if ((*s >= '0') && (*s <= '9'))
            r = (r << 4) + (*s - '0');
        else
        if ((*s >= 'a') && (*s <= 'f'))
            r = (r << 4) + (*s - 'a') + 10;
        else
        if ((*s >= 'A') && (*s <= 'F'))
            r = (r << 4) + (*s - 'A') + 10;
        else {
            err("arg[%d, symb:%d] not hex", _argc, i);
            break;
        }

    return r;
}

void CmdTxt::rep(const char *s, ...)
{
    va_list ap;
    const uint8_t x[] = { ':', '\n' };
    va_start (ap, s);

    int l = vsnprintf(NULL, 0, s, ap);
    char str[l+1];
    l = vsnprintf(str, l+1, s, ap);

    write(x, 1);
    write(reinterpret_cast<const uint8_t *>(id), strlen(id));
    write(x, 1);
    write(x, 1);

    write(reinterpret_cast<const uint8_t *>(str), l);

    write(x+1, 1);

    va_end (ap);
}

void CmdTxt::err(const char *s, ...) {
    va_list ap;
    va_start (ap, s);
    vsnprintf(_e, sizeof(_e), s, ap);
    va_end (ap);
}

void CmdTxt::fin() {
    const uint8_t x[] = { ':', '\n' };
    write(x, 1);
    write(reinterpret_cast<const uint8_t *>(id), strlen(id));
    write(x, 1);
    if (_e[0]) {
        write(reinterpret_cast<const uint8_t *>(_e), strlen(_e));
    }
    write(x+1, 1);
}
