
#include "buf.h"

#include <string.h>

buftx::buftx(size_t sz) :
    _data(reinterpret_cast<uint8_t *>(malloc(sz))),
    _bsz(sz),
    _c(0)
{}

buftx::buftx() :
    buftx(128)
{}

buftx::~buftx() {
    if (_data != NULL)
        free(_data);
}

bool buftx::skip(size_t sz) {
    if ((_c+sz) > _bsz)
        return false;
    bzero(_data+_c, sz);
    _c += sz;
    return true;
}

bool buftx::b(bool v) {
    return u8(v ? 1 : 0);
}

bool buftx::u8(uint8_t v) {
    if ((_c+1) > _bsz)
        return false;
    _data[_c] = v;
    _c ++;
    return true;
}

bool buftx::u16(uint16_t v) {
    if ((_c+2) > _bsz)
        return false;
    _data[_c]   = v >> 8;
    _data[_c+1] = v;
    _c += 2;
    return true;
}

bool buftx::u32(uint32_t v) {
    if ((_c+4) > _bsz)
        return false;
    _data[_c]   = v >> 24;
    _data[_c+1] = v >> 16;
    _data[_c+2] = v >> 8;
    _data[_c+3] = v;
    _c += 4;
    return true;
}

bool buftx::data(const uint8_t *d, size_t sz) {
    if ((_c+sz) > _bsz)
        return false;
    memcpy(_data+_c, d, sz);
    _c += sz;
    return true;
}

bufrx::bufrx(const uint8_t *d, size_t sz) :
    _data(d),
    _sz(sz)
{}

bool bufrx::skip(size_t sz) {
    if (_sz < sz)
        return false;
    _data += sz;
    _sz -= sz;
    return true;
}

bool bufrx::b(bool &v, bool deflt) {
    uint8_t vu;
    auto r = u8(vu);
    v = vu > 0;
    return r;
}

bool bufrx::u8(uint8_t &v, uint8_t deflt) {
    if (_sz < 1) {
        v = deflt;
        return false;
    }
    v = *_data;
    _data ++;
    _sz --;
    return true;
}

bool bufrx::d8(int8_t &v, int8_t deflt) {
    uint8_t u;
    bool r = u8(u, deflt);
    v = u;
    return r;
}

bool bufrx::u16(uint16_t &v, uint16_t deflt) {
    if (_sz < 2) {
        v = deflt;
        return false;
    }
    v = (static_cast<uint16_t>(_data[0]) << 8) | _data[1];
    _data   += 2;
    _sz     -= 2;
    return true;
}

bool bufrx::d16(int16_t &v, int16_t deflt) {
    uint16_t u;
    bool r = u16(u, deflt);
    v = u;
    return r;
}

bool bufrx::u16le(uint16_t &v, uint16_t deflt) {
    if (_sz < 2) {
        v = deflt;
        return false;
    }
    v = (static_cast<uint16_t>(_data[1]) << 8) | _data[0];
    _data   += 2;
    _sz     -= 2;
    return true;
}

bool bufrx::d16le(int16_t &v, int16_t deflt) {
    uint16_t u;
    bool r = u16le(u, deflt);
    v = u;
    return r;
}

bool bufrx::u24(uint32_t &v, uint32_t deflt) {
    if (_sz < 3) {
        v = deflt;
        return false;
    }
    v =
        (static_cast<uint32_t>(_data[0]) << 16) | 
        (static_cast<uint32_t>(_data[1]) << 8) | 
        _data[2];
    _data   += 3;
    _sz     -= 3;
    return true;
}

bool bufrx::d24(int32_t &v, int32_t deflt) {
    uint32_t u;
    bool r = u24(u, deflt);
    v = u;
    return r;
}

bool bufrx::u32(uint32_t &v, uint32_t deflt) {
    if (_sz < 4) {
        v = deflt;
        return false;
    }
    v =
        (static_cast<uint32_t>(_data[0]) << 24) |
        (static_cast<uint32_t>(_data[1]) << 16) | 
        (static_cast<uint32_t>(_data[2]) << 8) | 
        _data[3];
    _data   += 4;
    _sz     -= 4;
    return true;
}

bool bufrx::data(uint8_t *d, size_t sz) {
    if (_sz < sz)
        return false;
    memcpy(d, _data, sz);
    _data += sz;
    _sz -= sz;
    return true;
}

