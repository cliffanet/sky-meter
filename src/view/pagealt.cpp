/*
    Display: page alt page
*/

#include "page.h"
#include "btn.h"
#include "menu.h"
#include "../sys/log.h"

namespace Dspl {

static void _draw_alt(DSPL_ARG) {
    drawState(u8g2);
    drawClock(u8g2);
}


static uint8_t u = 0, s = 0, d = 0;
void pagealt() {
    set(_draw_alt);

    Btn::set(
        Btn::UP,
        [] () {
            CONSOLE("btn up smpl: %d", ++u);
        },
        [] () {
            CONSOLE("btn up long: %d", ++u);
        }
    );
    Btn::set(
        Btn::SEL,
        [] () {
            CONSOLE("btn sel smpl: %d", ++s);
        },
        Menu::activate
    );
    Btn::set(
        Btn::DN,
        NULL,
        [] () {
            CONSOLE("btn dn long: %d", ++d);
        }
    );
}

}; // namespace Dspl
