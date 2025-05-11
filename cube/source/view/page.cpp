/*
    Display: page elements
*/

#include "page.h"
#include "btn.h"
#include "menustatic.h"
#include "../sys/batt.h"
#include "../sys/clock.h"
#include "../jump/proc.h"
#include "../jump/saver.h"
#include "../uart/uart.h"

void Dspl::drawBatt(DSPL_ARG) {
    DSPL_COLOR(1);
    uint8_t v05 = batt::val05();
    if ((v05 > 0) || Dspl::isblink()) {
        DSPL_FONT(u8g2_font_battery19_tn);
        DSPL_FDIR(1);
        DSPL_GLYPH(0, 0, '0' + v05);
        DSPL_FDIR(0);
    }
    auto chrg = batt::charge();
    if (chrg > 0) {
        DSPL_FONT(u8g2_font_open_iconic_embedded_1x_t);
        DSPL_GLYPH(0, 20, 'C');
        if (chrg == batt::CHRG_HI)
            DSPL_GLYPH(10, 20, 'C');
    }
    
    //DSPL_FONT(u8g2_font_helvB08_tr);
    //DSPL_PRN(40, 10, "%d", batt::raw());
}

void Dspl::drawClock(DSPL_ARG) {
    auto tm = tmNow();
    char s[strsz];

    DSPL_COLOR(1);

    DSPL_FONT(u8g2_font_amstrad_cpc_extended_8n);
    sprn("%u:%02u", tm.h, tm.m);
    DSPL_STR(DSPL_S_RIGHT(s)-25, 10, s);
    
    DSPL_FONT(u8g2_font_blipfest_07_tn);
    sprn("%u.%02u.%04u", tm.day, tm.mon, tm.year);
    DSPL_STR(DSPL_S_RIGHT(s)-26, 16, s);
}

void Dspl::drawServ(DSPL_ARG) {
    if (jsave::isactive()) {
        DSPL_FONT(u8g2_font_open_iconic_www_1x_t);
        DSPL_GLYPH(30, 8, 'K');
    }
    if (uart::isactive()) {
        DSPL_FONT(u8g2_font_open_iconic_www_1x_t);
        DSPL_GLYPH(45, 8, 'O');
    }
}

void Dspl::page() {
    jmp::setdraw();

    Btn::set(
        Btn::UP,
        Dspl::lightTgl
    );
    Btn::set(
        Btn::SEL,
        NULL
#ifdef USE_MENU
        ,
        MenuStatic::main
#endif // USE_MENU
    );
    Btn::set(
        Btn::DN,
        jmp::pagenxt
    );
}
