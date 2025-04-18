/*
    buf
*/

#ifndef _buf_H
#define _buf_H

#ifdef __cplusplus

#include <stdint.h>
#include <stdlib.h>

class buftx {
    // изначала делал через template с возможностью указать размер буфера,
    // но оказалось, что так нельзя указать размер из переменной, только
    // жёстко из кода. При декларации char d[size] можно указать размер из переменной
    // и буфер будет в стеке, а через шаблон класса - нельзя, поэтому
    // возвращаемся к привычному malloc/free, т.к. размер буфера довольно
    // регулярно надо задавать из переменной, например при формировании пакетов со списком на отправку.
    uint8_t *_data;
    size_t _bsz, _c;
public:
    buftx();
    buftx(size_t sz);
    ~buftx();
    size_t sz() const { return _c; }
    const uint8_t * operator&() const { return _data; }

    bool skip(size_t sz = 1);
    bool b(bool v);
    bool u8(uint8_t v);
    bool u16(uint16_t v);
    bool u32(uint32_t v);
    bool data(const uint8_t *d, size_t sz);
    bool id(const uint8_t *d) {
        return data(d, 8);
    }
};

class bufrx {
    const uint8_t *_data;
    size_t _sz;
public:
    bufrx(const uint8_t *d, size_t sz);

    size_t sz() const { return _sz; }
    const uint8_t * operator&() const { return _data; }

    bool skip(size_t sz = 1);
    bool b(bool &v, bool deflt = false);
    bool u8(uint8_t &v, uint8_t deflt = 0);
    bool d8(int8_t &v, int8_t deflt = 0);
    bool u16(uint16_t &v, uint16_t deflt = 0);
    bool d16(int16_t &v, int16_t deflt = 0);
    bool u16le(uint16_t &v, uint16_t deflt = 0);
    bool d16le(int16_t &v, int16_t deflt = 0);
    bool u24(uint32_t &v, uint32_t deflt = 0);
    bool d24(int32_t &v, int32_t deflt = 0);
    bool u32(uint32_t &v, uint32_t deflt = 0);
    bool data(uint8_t *d, size_t sz);
    bool id(uint8_t *d) {
        return data(d, 8);
    }
};

#endif

#endif // _peer_H
