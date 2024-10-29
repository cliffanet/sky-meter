/*
    Display: Menu Static
*/

#include "menustatic.h"
#include "menumodal.h"
#include "text.h"
#include "../sys/log.h"

#include "../sys/maincfg.h"
#include "../view/btn.h"
#include "../view/dspl.h"


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
        prevstr(l, 1) ?
            strdup(l.name) :
            NULL;
    CONSOLE("sz: %d, title: %s", _sz, _title || "-null-");
}

MenuStatic::~MenuStatic() {
    if (_title != NULL)
        free(_title);
}

static void vnum(char *s, int v) {
    snprintf(s, MENUSZ_VAL, "%d", v);
}
static void vyesno(char *s, bool v) {
    strncpy(s, v ? TXT_MENU_YES : TXT_MENU_NO, MENUSZ_VAL);
}

static const MenuStatic::el_t _main[] = {
    {
        .name   = TXT_MAIN_JMPCNT,
        .enter  = [] { new MenuValInt(cfg->jmpcnt, [] (int v) { (*cfg)->jmpcnt = v; }, 0, 99999); },
        .showval= [] (char *v) { vnum(v, cfg->jmpcnt); }
    },
    {
        .name   = TXT_MAIN_FLIP180,
        .enter  = [] { new MenuValBool(cfg->flip180, [] (bool v) { Btn::flip180(v); Dspl::flip180(v); }); },
        .showval= [] (char *v) { vyesno(v, cfg->flip180); }
    },
    {
        .name   = TXT_MAIN_CONTRAST,
        .enter  = [] { new MenuValInt(cfg->contrast, [] (int v) { Dspl::contrast(v); }, 0, 20); },
        .showval= [] (char *v) { vnum(v, cfg->contrast); }
    },
    {
        .name   = TXT_MAIN_AUTOGND,
        .enter  = [] { new MenuValBool(cfg->autognd, [] (bool v) { (*cfg)->autognd = v; }); },
        .showval= [] (char *v) { vyesno(v, cfg->autognd); }
    },
    {
        .name   = TXT_MAIN_ALTMETER,
        .enter  = [] { new MenuValBool(cfg->altmeter, [] (bool v) { (*cfg)->altmeter = v; }); },
        .showval= [] (char *v) { vyesno(v, cfg->altmeter); }
    },
    {
        .name   = TXT_MAIN_ALTCORRECT,
        .enter  = [] { new MenuValInt(cfg->altcorrect, [] (int v) { (*cfg)->altcorrect = v; }, -5000, 10000); },
        .showval= [] (char *v) { vnum(v, cfg->altcorrect); }
    }
};

void MenuStatic::main() {
    MENU_STATIC(_main);
}
