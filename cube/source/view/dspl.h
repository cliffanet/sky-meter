/*
    Display
*/

#ifndef _view_display_H
#define _view_display_H

#include "../def.h"
#include "../u8g2/u8g2.h"
#include <stdio.h>

#define DSPL_ARG                u8g2_t *u8g2
#define DSPL_FONT(f)            u8g2_SetFont(u8g2, f)
#define DSPL_FDIR(v)            u8g2_SetFontDirection(u8g2, v)
#define DSPL_DWIDTH             u8g2_GetDisplayWidth(u8g2)
#define DSPL_DHEIGHT            u8g2_GetDisplayHeight(u8g2)
#define DSPL_COLOR(v)           u8g2_SetDrawColor(u8g2, v)

#define DSPL_GLYPH(x, y, c)     u8g2_DrawGlyph(u8g2, x, y, c)

#define DSPL_BOX(x, y, w, h)    u8g2_DrawBox(u8g2, x, y, w, h)
#define DSPL_FRAME(x, y, w, h)  u8g2_DrawFrame(u8g2, x, y, w, h)
#define DSPL_RFRAME(x, y, w, h, r) \
                                u8g2_DrawRFrame(u8g2, x, y, w, h, r)
#define DSPL_LINE(x1, y1, x2, y2) \
                                u8g2_DrawLine(u8g2, x1, y1, x2, y2)
#define DSPL_PIXEL(x, y)        u8g2_DrawPixel(u8g2, x, y)
#define DSPL_DISC(x, y, r)      u8g2_DrawDisc(u8g2, x, y, r, 15U)

#define DSPL_STR(x, y, s)       u8g2_DrawStr(u8g2, x, y, s)
#define DSPL_PRN(x, y, _s, ...) Dspl::prnstr(u8g2, x, y, s, ##__VA_ARGS__)


#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
#define DSPL_STRU(x, y, s)      u8g2_DrawUTF8(u8g2, x, y, s)
#define DSPL_PRNU(x, y, _s, ...) Dspl::prnutf(u8g2, x, y, s, ##__VA_ARGS__)
#define DSPL_S_WIDTH(s)         u8g2_GetUTF8Width(u8g2, s)
#include "text.ru.h"
#else
#define DSPL_STRU(x, y, s)      DSPL_STR(x, y, s)
#define DSPL_PRNU(x, y, s, ...) DSPL_STR(x, y, s, ##__VA_ARGS__)
#define DSPL_S_WIDTH(s)         u8g2_GetStrWidth(s)
#endif

#define DSPL_S_HEIGHT           u8g2_GetAscent(u8g2)
#define DSPL_S_RIGHT(s)         (DSPL_DWIDTH-DSPL_S_WIDTH(s))
#define DSPL_S_CENTER(s)        ((DSPL_DWIDTH-DSPL_S_WIDTH(s))/2)

#define strsz           32
#define sprn(ss, ...)    snprintf(s, strsz, ss, ##__VA_ARGS__)


namespace Dspl {

    typedef void (*draw_t)(u8g2_t *u8g2);
    typedef void (*tick_t)();

    void init();
    void set(draw_t draw, tick_t tick = NULL);

    void on();
    void off();
    void lightTgl();
    bool light();
    void contrast(uint8_t value);
    void flip180(bool flip);
    bool isblink();

    void prnstr(u8g2_t *u8g2, int x, int y, const char *s, ...);
    void prnutf(u8g2_t *u8g2, int x, int y, const char *s, ...);

    void tick();

}; // namespace Dspl

#endif // _view_display_H
