/*
    Display: Menu
*/

#include "menu.h"
#include "menustatic.h"
#include "menumodal.h"
#include "page.h"
#include "btn.h"
#include "text.h"
#include "../sys/worker.h"
#include "../sys/log.h"

#include <string.h>
#include <list>

class _menuWrk;
static _menuWrk* _w;

class _menuWrk : public Wrk {
    std::list<Menu *> _mall;
    Menu *_m = NULL;

    MenuModal *_modal = NULL, *_modal_del = NULL;

    bool _exit = false;
    uint32_t _tout = MENU_TIMEOUT;
    void _toutupd() {
        if (_tout > 0)
            _tout = MENU_TIMEOUT;
    }

    static void _draw(DSPL_ARG) {
        if (_w != NULL) {
            if (_w->_m != NULL)
                _w->_m->draw(u8g2);
            if (_w->_modal != NULL)
                _w->_modal->draw(u8g2);
        }
    }
    static void _smplup() {
        if ((_w != NULL) && (_w->_m != NULL)) {
            _w->_toutupd();
            if (_w->_modal != NULL)
                _w->_modal->smplup();
            else
                _w->_m->smplup();
        }
    }
    static void _smpldn() {
        if ((_w != NULL) && (_w->_m != NULL)) {
            _w->_toutupd();
            if (_w->_modal != NULL)
                _w->_modal->smpldn();
            else
                _w->_m->smpldn();
        }
    }
    static void _smplsel() {
        if ((_w != NULL) && (_w->_m != NULL)) {
            _w->_toutupd();
            if (_w->_modal != NULL)
                _w->_modal->smplsel();
            else
                _w->_m->smplsel();
        }
    }
public:
    _menuWrk() {
        _w = this;
        optset(O_AUTODELETE);

        Dspl::set(_draw);
        Btn::set(Btn::UP,   _smplup);
        Btn::set(Btn::DN,   _smpldn);
        Btn::set(Btn::SEL,  _smplsel);
    }
    ~_menuWrk() {
        CONSOLE("destroy 0x%08x", this);
        _m = NULL;
        for (auto m: _mall)
            delete m;
        if (_modal != NULL)
            delete _modal;
        if (_modal_del != NULL)
            delete _modal_del;
        Dspl::page();
        if (_w == this)
            _w = NULL;
    }

    Menu *prev() {
        if (_mall.size() < 2)
            return NULL;
        auto it = _mall.begin();
        it++;
        return *it;
    }

    void add(Menu *m) {
        CONSOLE("menu enter: 0x%08x", m);
        _mall.push_front(m);
        _m = m;
    }
    /*
    void del(Menu *m) {
        for (auto it = _mall.begin(); it != _mall.end(); it++)
            if (*it == m) {
                _mall.erase(it);
                CONSOLE("found");
                break;
            }
        _m = m;
    }
    */

    void mexit() {
        _exit = true;
    }

    void modalset(MenuModal *modal) {
        modalclose();
        _modal = modal;
        CONSOLE("set modal: 0x%08x", modal);
    }
    void modalclose() {
        if (_modal_del != NULL)
            delete _modal_del;
        _modal_del = _modal;
        _modal = NULL;
    }

    state_t run() {
        if (_exit) {
            _exit = false;
            
            if (!_mall.empty()) {
                auto *m = _mall.front();
                CONSOLE("menu exit: 0x%08x", m);
                _mall.erase(_mall.begin());
                delete m;
            }
            if (_mall.empty()) {
                _m = NULL;
                return END;
            }

            _m = _mall.front();
        }

        if (_tout > 0) {
            _tout--;
            if (_tout == 0) {
                CONSOLE("timeout");
                return END;
            }
        }

        if (_modal_del != NULL) {
            CONSOLE("clear modal: 0x%08x", _modal_del);
            delete _modal_del;
            _modal_del = NULL;
        }

        return DLY;
    }
};

Menu::Menu(exit_t _exit) :
    _exit(_exit)
{
    if (_w == NULL)
        _w = new _menuWrk();
    if (_w != NULL)
        _w->add(this);
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
        uint8_t i = n + _itop;          // i - индекс отрисовываемого пункта меню
        if (i >= _sz) break;            // для меню, которые полностью отрисовались и осталось ещё место, не выходим за список меню
        //DSPL_COLOR(_isel == i ? 0 : 1); // Выделенный пункт рисуем прозрачным, остальные обычным
        int8_t y = (n+1)*10-1+14;       // координата y для отрисовки текста (в нескольких местах используется)
        
        // Получение текстовых данных текущей строки
        // Раньше мы эти строчки кешировали в массиве, обновляя
        // этот массив каждый раз при смещении меню, но
        // в итоге от этого кеширования отказались, т.к. часто
        // данные требуют оперативного обновления, а затраты на
        // их получение минимальны, при этом очень много гемора,
        // связанного с обновлением кеша
        line_t m;
        bool isexit = ((_exit == EXIT_TOP) && (i == 0)) || ((_exit == EXIT_BOTTOM) && (i+1 >= _sz));
        if (isexit) {
            strncpy(m.name, TXT_MENU_EXIT, sizeof(m.name));
            m.name[sizeof(m.name)-1] = '\0';
            m.val[0] = '\0';
        }
        else {
            str(m, _exit == EXIT_TOP ? i-1 : i);
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
    bool isexit = ((_exit == EXIT_TOP) && (_isel == 0)) || ((_exit == EXIT_BOTTOM) && (_isel+1 >= _sz));
    if (isexit) {
        if (_w != NULL)
            _w->mexit();
    }
    else
        onsel(_exit == EXIT_TOP ? _isel-1 : _isel);
}

bool Menu::prevstr(line_t &s) {
    if (_w == NULL)
        return false;
    auto m = _w->prev();
    if (m == NULL)
        return false;
    
    m->str(s, m->_exit == EXIT_TOP ? m->_isel-1 : m->_isel);

    return true;
}

void Menu::modalset(MenuModal *m) {
    if (_w != NULL)
        _w->modalset(m);
    else
        delete m;
}

void Menu::modalclose() {
    if (_w != NULL)
        _w->modalclose();
}
