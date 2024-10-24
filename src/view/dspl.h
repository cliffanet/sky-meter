/*
    Display
*/

#ifndef _view_display_H
#define _view_display_H

#include "../../def.h"
#include "../u8g2/u8g2.h"
#include <stdio.h>

#define DSPL_ARG                u8g2_t *u8g2
#define DSPL_FONT(f)            u8g2_SetFont(u8g2, f)
#define DSPL_FDIR(v)            u8g2_SetFontDirection()(u8g2, v)
#define DSPL_COLOR(v)           u8g2_SetDrawColor(u8g2, v)
#define DSPL_GLYPH(x, y, c)     u8g2_DrawGlyph(u8g2, x, y, c)
#define DSPL_STR(x, y, s)       u8g2_DrawStr(u8g2, x, y, s);
#define DSPL_PRN(x, y, _s, ...) \
        do { char s[48]; snprintf(s, sizeof(s), _s, ##__VA_ARGS__); u8g2_DrawStr(u8g2, x, y, s); } while (0)

namespace Dspl {

    typedef void (*draw_t)(u8g2_t *u8g2);

    void init();
    void set(draw_t draw);

    void on();
    void off();
    void lightTgl();
    bool light();
    uint8_t contrast();
    void contrast(uint8_t value);
    bool flip180();
    void flip180(bool flip);

}; // namespace Dspl

#endif // _view_display_H
