/*
    Display: page elements
*/

#include "page.h"
#include "../sys/clock.h"

void Dspl::drawState(DSPL_ARG) {
    /*
    DSPL_COLOR(1);
    uint8_t blev = pwrBattLevel();
    if ((blev > 0) || isblink()) {
        DSPL_FONT(u8g2_font_battery19_tn);
        DSPL_FDIR(1);
        DSPL_GLYPH(0, 0, '0' + blev);
        DSPL_FDIR(0);
    }
    if (pwrBattCharge()) {
        DSPL_FONT(u8g2_font_open_iconic_embedded_1x_t);
        DSPL_GLYPH(0, 20, 'C');
    }
    
    DSPL_FONT(u8g2_font_helvB08_tr);
    */
}

void Dspl::drawClock(DSPL_ARG) {
    auto tm = tmNow();

    DSPL_COLOR(1);
    DSPL_FONT(u8g2_font_amstrad_cpc_extended_8n);
    DSPL_PRN(80, 10, "%d:%02d", tm.h, tm.m);
}

void Dspl::page() {
    pagealt();
}
