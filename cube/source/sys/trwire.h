/*
    Transfere Wire
*/

#ifndef _sys_trwire_H
#define _sys_trwire_H

#include <stdint.h>
#include <stdio.h>

// выполнять команду read одним блоком передачи,
// для этого формируется один большой буфер tx,
// включающий отправляемую команду + количество
// принимаемых байт, заполненных FF и всё это
// отправляется в SPI одним буфером.
// Потом из приёмного буфера нужный блок копируется
// в буфер-получения. Как видим, тут очень много
// действий по копированию данных.
#define TRANSSPI_READMONO
// Без этого флага сначала отправляется команда,
// а потом отдельно читаются данные сразу
// в буфер-приёмник.

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
