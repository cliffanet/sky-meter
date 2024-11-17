/*
    Display: Menu
*/

#include "menu.h"

#ifdef USE_MENU
#include "menustatic.h"
#include "menumodal.h"
#include "page.h"
#include "btn.h"
#include "text.h"
#include "../sys/maincfg.h"
#include "../sys/log.h"

#include <string.h>
#include <list>

static Menu *_menu = NULL;
static MenuModal *_modal = NULL;

static uint32_t _tout = MENU_TIMEOUT;
static void _toutupd() {
    if (_tout > 0)
        _tout = MENU_TIMEOUT;
}

static void _draw(DSPL_ARG) {
    if (_menu != NULL)
        _menu->draw(u8g2);
    if (_modal != NULL)
        _modal->draw(u8g2);
}
static void _smplup() {
    _toutupd();
    if (_modal != NULL)
        _modal->smplup();
    else
    if (_menu != NULL)
        _menu->smplup();
}
static void _smpldn() {
    _toutupd();
    if (_modal != NULL)
        _modal->smpldn();
    else
    if (_menu != NULL)
        _menu->smpldn();
}
static void _smplsel() {
    _toutupd();
    if (_modal != NULL)
        _modal->smplsel();
    else
    if (_menu != NULL)
        _menu->smplsel();
}

static void _tick() {
    if (_tout > 0) {
        _tout--;
        if (_tout == 0) {
            CONSOLE("timeout");
            Menu::clear();
        }
    }
}

Menu::Menu(exit_t _exit) : 
    _exit(_exit)
{
    CONSOLE("new 0x%08x", this);

    _prv = _menu;
    if (_menu != NULL)
        _menu->_nxt = this;
    _menu = this;
    _tout = MENU_TIMEOUT;

    Dspl::set(_draw, _tick);
    Btn::set(Btn::UP,   _smplup);
    Btn::set(Btn::DN,   _smpldn);
    Btn::set(Btn::SEL,  _smplsel);
}

Menu::~Menu() {
    CONSOLE("destroy 0x%08x", this);

    if (_menu == this)
        _menu = _prv;
    if (_prv != NULL)
        _prv->_nxt = _nxt;
    if (_nxt != NULL)
        _nxt->_prv = _prv;
    
    if (_menu == NULL)
        clear();
}

int16_t Menu::ipos(int16_t i) {
    switch (_exit) {
        case EXIT_NONE:
            return
                (i >= 0) && (i < static_cast<int16_t>(sz())) ?
                    i : -2;
        case EXIT_TOP:
            i--;
            return (i >= -1) && (i < static_cast<int16_t>(sz())) ? i : -2;

        case EXIT_BOTTOM:
            return
                (i >= 0) && (i < static_cast<int16_t>(sz())) ?
                    i :
                (i == static_cast<int16_t>(sz())) ?
                    -1 : -2;
    }

    return -2;
}

void Menu::close() {
    delete this;
}

#define _sz     (_exit == EXIT_NONE ? sz() : sz()+1)

void Menu::draw(DSPL_ARG) {
    DSPL_FONT(menuFont);
    
    // Заголовок
    char t[MENUSZ_TITLE];
    title(t);
    t[sizeof(t)-1] = '\0';
    DSPL_BOX(0, 0, DSPL_DWIDTH, 12);
    DSPL_COLOR(0);
    DSPL_STRU(DSPL_S_CENTER(t), 10, t);
    
    // выделение пункта меню, текст будем писать поверх
    DSPL_COLOR(1);
    DSPL_FRAME(0, (_isel-_itop)*10+14, DSPL_DWIDTH, 10);
    
    // Выводим видимую часть меню, n - это индекс строки на экране
    for (uint8_t n = 0; n<MENU_STR_COUNT; n++) {
        auto i = ipos(_itop + n);    // i - индекс отрисовываемого пункта меню
        if (i < -1) break;              // для меню, которые полностью отрисовались и осталось ещё место, не выходим за список меню
        int8_t y = (n+1)*10-1+14;       // координата y для отрисовки текста (в нескольких местах используется)
        
        // Получение текстовых данных текущей строки
        // Раньше мы эти строчки кешировали в массиве, обновляя
        // этот массив каждый раз при смещении меню, но
        // в итоге от этого кеширования отказались, т.к. часто
        // данные требуют оперативного обновления, а затраты на
        // их получение минимальны, при этом очень много гемора,
        // связанного с обновлением кеша
        line_t m = { 0 };
        if (i < 0) {
            strncpy(m.name, TXT_MENU_EXIT, sizeof(m.name));
            m.name[sizeof(m.name)-1] = '\0';
            m.val[0] = '\0';
        }
        else {
            str(m, i);
            m.name[sizeof(m.name)-1] = '\0';
            m.val[sizeof(m.val)-1] = '\0';
        }

        DSPL_STRU(2, y, m.name);
        if (m.val[0] != '\0')   // вывод значения
            DSPL_STRU(DSPL_S_RIGHT(m.val)-2, y, m.val);
    }
    DSPL_COLOR(1);
}

void Menu::smplup() {
    _isel --;
    if (_isel >= 0) {
        if (_itop > _isel)  // если вылезли вверх за видимую область,
            _itop = _isel;  // спускаем список ниже, чтобы отобразился выделенный пункт
    }
    else {                  // если упёрлись в самый верх, перескакиваем в конец меню
        _isel = _sz-1;
        if (_sz > MENU_STR_COUNT)
            _itop = _sz-MENU_STR_COUNT;
    }
}

void Menu::smpldn() {
    _isel ++;
    if (_isel < _sz) {
        // если вылезли вниз за видимую область,
        // поднимаем список выше, чтобы отобразился выделенный пункт
        if ((_isel > _itop) && ((_isel-_itop) >= MENU_STR_COUNT))
            _itop = _isel-MENU_STR_COUNT+1;
    }
    else { // если упёрлись в самый низ, перескакиваем в начало меню
        _isel = 0;
        if (_sz > MENU_STR_COUNT)
            _itop = 0;
    }
}

void Menu::smplsel() {
    auto i = ipos(_isel);
    if (i < 0)
        close();
    else
        onsel(i);
}

Menu *Menu::prev(uint8_t n) {
    auto m = _menu;
    for (; (m != NULL) && (n > 0); n--)
        m = m->_prv;
    return m;
}

bool Menu::prevstr(line_t &s, uint8_t n) {
    auto m = prev(n);
    if (m == NULL)
        return false;
    
    m->str(s, m->_exit == EXIT_TOP ? m->_isel-1 : m->_isel);

    return true;
}

bool Menu::isactive() {
    return _menu != NULL;
}

void Menu::clear() {
    while (_menu != NULL)
        delete _menu;
    if (_modal != NULL)
        delete _modal;
    cfg.save();
    Dspl::page();
}

void Menu::modalset(MenuModal *m) {
    if ((_modal != NULL) && (_modal != m))
        delete _modal;
    _modal = m;
    CONSOLE("set modal: 0x%08x", m);
}

void Menu::modaldel(MenuModal *m) {
    if (_modal == m)
        _modal = NULL;
}

#endif // USE_MENU
