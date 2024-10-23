/*
    Display
*/

#include "dspl.h"
#include "../sys/stm32drv.h"
#include "../sys/worker.h"
#include "../sys/log.h"

#define DSPL_PIN_DC     GPIOB, GPIO_PIN_2
#define DSPL_PIN_RST    GPIOB, GPIO_PIN_1
#define DSPL_PIN_CS     GPIOB, GPIO_PIN_11

extern SPI_HandleTypeDef hspi1;

static uint32_t n = 0;
static void draw_test(DSPL_ARG) {
    DSPL_FONT(u8g2_font_ncenB12_tr);
    DSPL_PRN(20, 20, "n: %d", n);
}

class _wrkDspl : public Wrk {
    u8g2_t u8g2;

    Dspl::draw_t _draw = NULL;
    bool _clear = true;

    static uint8_t _4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
        switch (msg) {
            case U8X8_MSG_BYTE_SEND:
                HAL_SPI_Transmit(&hspi1, (uint8_t *) arg_ptr, arg_int, 10000);
                break;
            case U8X8_MSG_BYTE_INIT:
                HAL_GPIO_WritePin(DSPL_PIN_CS, u8x8->display_info->chip_disable_level ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            case U8X8_MSG_BYTE_SET_DC:
                HAL_GPIO_WritePin(DSPL_PIN_DC, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            case U8X8_MSG_BYTE_START_TRANSFER:
                HAL_GPIO_WritePin(DSPL_PIN_CS, u8x8->display_info->chip_enable_level ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            case U8X8_MSG_BYTE_END_TRANSFER:
                HAL_GPIO_WritePin(DSPL_PIN_CS, u8x8->display_info->chip_disable_level ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            default:
                return 0;
        }
        return 1;
    }
    
    static uint8_t _gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,
        U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,
        U8X8_UNUSED void *arg_ptr)
    {
        switch (msg) {
            case U8X8_MSG_GPIO_AND_DELAY_INIT:
                HAL_Delay(1);
                break;
            case U8X8_MSG_DELAY_MILLI:
                HAL_Delay(arg_int);
                break;
            case U8X8_MSG_GPIO_CS:
                HAL_GPIO_WritePin(DSPL_PIN_CS, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            case U8X8_MSG_GPIO_DC:
                HAL_GPIO_WritePin(DSPL_PIN_DC, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            case U8X8_MSG_GPIO_RESET:
                HAL_GPIO_WritePin(DSPL_PIN_RST, arg_int ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
        }

        return 1;
    }

public:
    _wrkDspl() {
        u8g2_Setup_st75256_jlx19296_f(&u8g2, U8G2_R0, _4wire_hw_spi, _gpio_and_delay);
        u8g2_InitDisplay(&u8g2);
        u8g2_SetPowerSave(&u8g2, 0);
        _draw = draw_test;
        CONSOLE("init");
    }
    void draw(Dspl::draw_t _d) {
        u8g2_FirstPage(&u8g2);
        do {
            _d(&u8g2);
        } while (u8g2_NextPage(&u8g2));
    }
    void set(Dspl::draw_t _d) {
        _clear = true;
        _draw = _d;
    }

    state_t run () {
        if (_clear) {
            _clear = false;
            u8g2_ClearDisplay(&u8g2);
        }

        if (_draw != NULL)
            draw(_draw);
        
        n++;

        return DLY;
    }
};

namespace Dspl {

    static _wrkDspl *_w = NULL;

void init() {
    if (_w != NULL)
        return;
    _w = new _wrkDspl();
}

void set(draw_t draw) {
    if (_w != NULL)
        _w->set(draw);
}

}; // namespace Dspl
