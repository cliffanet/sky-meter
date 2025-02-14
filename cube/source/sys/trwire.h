/*
    Transfere Wire
*/

#ifndef _sys_trwire_H
#define _sys_trwire_H

#include <stdint.h>
#include <stdio.h>

class TransWire {
    public:
        virtual
        bool init() { return true; };
        virtual
        uint8_t read8(uint8_t reg) = 0;
        virtual
        bool write8(uint8_t reg, uint8_t val) = 0;
        virtual
        bool read(uint8_t reg, uint8_t *d, size_t sz) = 0;
        virtual
        bool write(uint8_t reg, const uint8_t *d, size_t sz) = 0;
};

class TransSPI : public TransWire {
    void *_iox;
    uint16_t _pin;

    void transbeg();
    void transend();

    public:
        TransSPI(void *_iox, uint16_t _pin);
        
        bool init();

        uint8_t read8(uint8_t reg);
        bool write8(uint8_t reg, uint8_t val);
        bool read(uint8_t reg, uint8_t *d, size_t sz);
        bool write(uint8_t reg, const uint8_t *d, size_t sz);
};

#endif // _sys_trwire_H
