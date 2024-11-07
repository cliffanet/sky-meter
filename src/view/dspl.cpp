/*
    Display
*/

#include "dspl.h"
#include "page.h"
#include "../sys/stm32drv.h"
#include "../sys/maincfg.h"
#include "../sys/log.h"

#define DSPL_PIN_DC     GPIOB, GPIO_PIN_2
#define DSPL_PIN_RST    GPIOB, GPIO_PIN_1
#define DSPL_PIN_CS     GPIOB, GPIO_PIN_11
#define DSPL_PIN_LGHT   GPIOA, GPIO_PIN_10

static bool _lght = false;
static void _lghtUpd() {
    HAL_GPIO_WritePin(DSPL_PIN_LGHT, _lght ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

extern SPI_HandleTypeDef hspi1;

static u8g2_t u8g2;

static Dspl::draw_t _draw = NULL;
static Dspl::tick_t _tick = NULL;
static bool _clear = true;

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

namespace Dspl {

void init() {
    u8g2_Setup_st75256_jlx19296_f(&u8g2, U8G2_R0, _4wire_hw_spi, _gpio_and_delay);
    on();
}

void set(draw_t draw, tick_t tick) {
    _clear = true;
    _draw = draw;
    _tick = tick;
}

void on() {
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearDisplay(&u8g2);

    contrast(cfg->contrast);
    flip180(cfg->flip180);
    _lghtUpd();

    page();
}

void off() {
    u8g2_SetPowerSave(&u8g2, 1);
    HAL_GPIO_WritePin(DSPL_PIN_LGHT, GPIO_PIN_RESET);
}

void lightTgl() {
    _lght = not _lght;
    _lghtUpd();
}

bool light() {
    return _lght;
}

void contrast(uint8_t value) {
    if (cfg->contrast != value)
        (*cfg)->contrast = value;
    u8g2_SetContrast(&u8g2, 115+value);
}

void flip180(bool flip) {
    if (cfg->flip180 != flip)
        (*cfg)->flip180 = flip;
    u8g2_SetFlipMode(&u8g2, flip ? 1 : 0);
}

void tick() {
    if (_clear) {
        _clear = false;
        u8g2_ClearDisplay(&u8g2);
    }

    if (_tick != NULL)
        _tick();

    u8g2_FirstPage(&u8g2);
    do {
        if (_draw != NULL)
            _draw(&u8g2);
    } while (u8g2_NextPage(&u8g2));
}

}; // namespace Dspl
