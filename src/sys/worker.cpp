/*
    Worker
*/

#include "worker.h"

#include <list> // list более экономный, чем map

#include "log.h" // временно для отладки

////////////////////////////////////////////////////////////////////////////////

static std::list<Wrk *> _wrkall;

Wrk::Wrk() {
    CONSOLE("Wrk(0x%08x) begin", this);
    _wrkall.push_back(this);
    optset(Wrk::O_INLIST);
}

Wrk::~Wrk() {
    CONSOLE("Wrk(0x%08x) destroy", this);
    for (auto p = _wrkall.begin(); p != _wrkall.end(); p++)
        if (*p == this) {
            CONSOLE("found");
            _wrkall.erase(p);
            break;
        }
}

////////////////////////////////////////////////////////////////////////////////

bool wrkEmpty() {
    return true;//_wrkall.empty();
}

extern "C"
uint32_t mill();

extern "C"
uint32_t wrkProcess(uint32_t msmax) {
    if (_wrkall.size() == 0)
        return 0;
    
    for (auto &w : _wrkall) {
        // сбрасываем флаг needwait
        if (w->opt(Wrk::O_NEEDWAIT))
            w->optdel(Wrk::O_NEEDWAIT);
        // timer
        w->timer();
    }
    
    uint32_t beg = mill();
    bool run = true;
    
    while (run) {
        run = false;
        for(auto it = _wrkall.begin(), itnxt = it; it != _wrkall.end(); it = itnxt) {
            itnxt++; // такие сложности, чтобы проще было удалить текущий элемент
            auto &w = *it;

            if ( w->opt(Wrk::O_NEEDFINISH) ) {
                if (!w->opt(Wrk::O_FINISHED))
                    w->end();

                if (w->opt(Wrk::O_AUTODELETE)) {
                    delete w; // при выполнении delete - _wrkall.erase выполнится само в деструкторе Wrk
                }
                else {
                    w->optclr(Wrk::O_FINISHED);
                    w->optclr(Wrk::O_INLIST);
                    _wrkall.erase(it);
                }

                CONSOLE("Wrk(0x%08x) finished", w);
            }
            else
            if ( ! w->opt(Wrk::O_NEEDWAIT) )
                switch ( w->run() ) {
                    case Wrk::DLY:
                        w->optset(Wrk::O_NEEDWAIT);
                        break;

                    case Wrk::RUN:
                        run = true;
                        break;

                    case Wrk::END:
                        w->optset(Wrk::O_NEEDFINISH);
                        run = true;
                        break;
                }
            
            uint32_t ms = mill() - beg;
            if (ms >= msmax)
                return ms;
        }
    }

    return mill() - beg;
}
