/*
    Checksumm calculator
*/

#ifndef _sys_cks_H
#define _sys_cks_H

#include <stdint.h>
#include <stddef.h>

typedef struct __attribute__((__packed__)) cks_s {
    uint8_t a, b;

    cks_s() :
        a(0),
        b(0)
    { }
    cks_s(const uint8_t *d, size_t _sz) :
        cks_s()
    { add(d, _sz); }

    template <typename T>
    cks_s(const T &d) :
        cks_s()
    { add(reinterpret_cast<const uint8_t *>(&d), sizeof(T)); }

    bool operator== (const struct cks_s & cks) {
        return (this == &cks) || ((this->a == cks.a) && (this->b == cks.b));
    };
    operator bool() const { return (a != 0) || (b != 0); }

    void clear() {
        a = 0;
        b = 0;
    }

    void addi(uint8_t d) {
        a += d;
        b += a;
    }

    void add(const uint8_t *d, size_t _sz) {
        for (; _sz > 0; _sz--, d++)
            addi(*d);
    }

    template <typename T>
    void add(const T &d) {
        add(reinterpret_cast<const uint8_t *>(&d), sizeof(T));
    }
} cks_t;

#endif // _sys_cks_H
