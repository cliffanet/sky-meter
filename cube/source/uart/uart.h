/*
    UART
*/

#ifndef _uart_H
#define _uart_H

#include <stdint.h>


#ifdef __cplusplus
namespace uart {
    bool isactive();
    void tick();
}
#endif

extern "C"
void uartrecv(const uint8_t *data, uint32_t sz);


#endif // _uart_H
