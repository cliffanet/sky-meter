/*
    Display
*/

#include "dspl.h"
#include "page.h"
#include "../sys/stm32drv.h"
#include "../sys/maincfg.h"
#include "../sys/power.h"
#include "../sys/log.h"

#if HWVER < 2
#define DSPL_PIN_DC     GPIOB, GPIO_PIN_2
#define DSPL_PIN_RST    GPIOB, GPIO_PIN_1
#define DSPL_PIN_CS     GPIOB, GPIO_PIN_11
#define DSPL_PIN_LGHT   GPIOA, GPIO_PIN_10
#else
#define DSPL_PIN_EN     GPIOB, GPIO_PIN_15
#define DSPL_PIN_DC     GPIOA, GPIO_PIN_9
#define DSPL_PIN_RST    GPIOA, GPIO_PIN_10
#define DSPL_PIN_CS     GPIOA, GPIO_PIN_8
#define DSPL_PIN_LGHT   GPIOA, GPIO_PIN_15
#endif // HWVER

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
    pwr::hwen(true);

    /*
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    */

    HAL_GPIO_WritePin(DSPL_PIN_RST,  GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(DSPL_PIN_RST,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(DSPL_PIN_CS,   GPIO_PIN_SET);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    contrast(cfg->contrast);
    flip180(cfg->flip180);
    _lghtUpd();

    page();
    tick();
}

void off() {
    u8g2_SetPowerSave(&u8g2, 1);
    HAL_GPIO_WritePin(DSPL_PIN_LGHT, GPIO_PIN_RESET);
    // попытка прибить пины к земле при выключении
    // добавляет потребление 50 мкА
    /*
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    */
    //HAL_GPIO_WritePin(DSPL_PIN_DC,   GPIO_PIN_RESET);
    //HAL_GPIO_WritePin(DSPL_PIN_RST,  GPIO_PIN_RESET);
    pwr::hwen(false);
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

uint8_t _blink = 0;
bool isblink() {
    return (_blink & 0x08) == 0;
}

void prnstr(u8g2_t *u8g2, int x, int y, const char *s, ...) {
    va_list ap;
    char str[128];
    va_start (ap, s);
    vsnprintf(str, sizeof(str), s, ap);
    va_end (ap);

    u8g2_DrawStr(u8g2, x, y, str);
}

void prnutf(u8g2_t *u8g2, int x, int y, const char *s, ...) {
    va_list ap;
    char str[128];
    va_start (ap, s);
    vsnprintf(str, sizeof(str), s, ap);
    va_end (ap);

    u8g2_DrawUTF8(u8g2, x, y, str);
}

void tick() {
    _blink ++;

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
