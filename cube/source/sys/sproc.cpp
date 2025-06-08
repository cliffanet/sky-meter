/*
    Dynamic process manager
*/

#include "sproc.h"

#include <vector>

static std::vector<proc::elem_t> _all;
static std::vector<proc::elem_t>::iterator _nxt = _all.end();

namespace proc {
    void add(elem_t proc, bool multi) {
        if (!multi)
            for (auto p : _all)
                if (p == proc)
                    return;
        _all.push_back(proc);
        _nxt = _all.begin();
    }

    bool isactive() {
        return _all.size() > 0;
    }

    void run() {
        if (_all.empty())
            return;

        if (_nxt == _all.end())
            _nxt = _all.begin();
        auto _cur = _nxt;
        _nxt++;

        if (!(*_cur)())
            _all.erase(_cur);
    }
}
