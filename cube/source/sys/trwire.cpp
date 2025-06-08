/*
    Transfere Wire
*/

#include "stm32drv.h"
#include "trwire.h"

#include <string.h>

extern SPI_HandleTypeDef hspi1;

TransSPI::TransSPI(void *_iox, uint16_t _pin) : 
    _iox(_iox),
    _pin(_pin)
{ }

bool TransSPI::init() {
    transend();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = _pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(reinterpret_cast<GPIO_TypeDef *>(_iox), &GPIO_InitStruct);
    return true;
}

void TransSPI::transbeg() {
    HAL_GPIO_WritePin(reinterpret_cast<GPIO_TypeDef *>(_iox), _pin, GPIO_PIN_RESET);
}

void TransSPI::transend() {
    HAL_GPIO_WritePin(reinterpret_cast<GPIO_TypeDef *>(_iox), _pin, GPIO_PIN_SET);
}

uint8_t TransSPI::read8(uint8_t reg) {
    // 0x80 - read operation, 0xFF - dummy byte for response
    uint8_t tx[2] = { static_cast<uint8_t>(reg | 0x80), 0xFF };
    uint8_t rx[2];
    transbeg();
    auto r = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 20);
    transend();

    return r == HAL_OK ? rx[1] : 0x00;
}

bool TransSPI::write8(uint8_t reg, uint8_t val) {
    // 0x80 - read operation, 0xFF - dummy byte for response
    uint8_t tx[2] = { static_cast<uint8_t>(reg & ~0x80), val };
    transbeg();
    auto r = HAL_SPI_Transmit(&hspi1, tx, 2, 20);
    transend();

    return r == HAL_OK;
}

bool TransSPI::read(uint8_t reg, uint8_t *d, size_t sz) {
#ifdef TRANSSPI_READMONO
    uint8_t tx[sz+1], rx[sz+1];
    tx[0] = reg | 0x80;
    memset(tx+1, 0xff, sz);

    transbeg();
    auto r = HAL_SPI_TransmitReceive(&hspi1, tx, rx, sz+1, 20);
    transend();

    memcpy(d, rx+1, sz);
#else
    transbeg();
    reg |= 0x80;
    uint8_t /*txr = reg | 0x80,*/ rxr;
    auto r = HAL_SPI_TransmitReceive(&hspi1, &reg, &rxr, 1, 10);
    if (r == HAL_OK) {
        uint8_t tx[sz];
        memset(tx, 0xFF, sz);
        r = HAL_SPI_TransmitReceive(&hspi1, tx, d, sz, 10);
    }
    transend();
#endif

    return r == HAL_OK;
}

bool TransSPI::write(uint8_t reg, const uint8_t *d, size_t sz) {
    uint8_t tx[sz+1];
    tx[0] = reg & ~0x80;
    memcpy(tx+1, d, sz);

    transbeg();
    auto r = HAL_SPI_Transmit(&hspi1, tx, sz, 20);
    transend();

    return r == HAL_OK;
}
