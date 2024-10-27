/*
    Display: Menu Static
*/

#include "menustatic.h"
#include "menumodal.h"
#include "text.h"
#include "../sys/log.h"

#include <stdlib.h>
#include <string.h>

void MenuStatic::title(char *s) {
    strncpy(s, _title, MENUSZ_TITLE);
}

void MenuStatic::str(line_t &s, int16_t i) {
    auto &m = _m[i];

    strncpy(s.name, m.name, sizeof(s.name));
    
    if (m.showval == NULL)
        s.val[0] = '\0';
    else
        m.showval(s.val);
}

void MenuStatic::onsel(int16_t i) {
    auto &m = _m[i];

    if (m.enter != NULL)
        m.enter();
}

MenuStatic::MenuStatic(const el_t *m, int16_t sz) :
    _m(m),
    _sz(sz)
{
    line_t l;
    _title =
        prevstr(l) ?
            strdup(l.name) :
            NULL;
    CONSOLE("sz: %d, title: %s", _sz, _title || "-null-");
}

MenuStatic::~MenuStatic() {
    if (_title != NULL)
        free(_title);
}

static const MenuStatic::el_t _mm[] = {
    {
        .name   = "menu sub 1",
        .enter  = [] { CONSOLE("menu s 1"); },
    },
    {
        .name   = "menu sub 2",
        .enter  = [] { CONSOLE("menu s 2"); },
    }
};

static const MenuStatic::el_t _main[] = {
    {
        .name   = "menu 1",
        .enter  = [] { CONSOLE("menu 1"); new MenuConfirm("hello?", NULL); },
        .showval= [] (char *v) {
            strncpy(v, "test", MENUSZ_VAL);
        }
    },
    {
        .name   = "menu 2",
        .enter  = [] { CONSOLE("menu 2"); MENU_STATIC(_mm); },
        .showval= [] (char *v) {
            strncpy(v, "test2", MENUSZ_VAL);
        }
    }
};

void MenuStatic::main() {
    MENU_STATIC(_main);
}
