/*
    Display: page alt page
*/

#include "page.h"

namespace Dspl {

static void _draw_alt(DSPL_ARG) {
    drawState(u8g2);
    drawClock(u8g2);
}

void pagealt() {
    set(_draw_alt);
}

}; // namespace Dspl
