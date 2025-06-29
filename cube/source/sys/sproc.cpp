/*
    Dynamic process manager
*/

#include "sproc.h"

#include <vector>
#include <set>

typedef struct {
    proc::elem_t proc;
    uint32_t id;
} _item_t;

static std::vector<_item_t> _all;
static std::vector<_item_t>::iterator _nxt = _all.end();

static uint32_t _id = 0;
static std::set<uint32_t> _idall;
static uint32_t idgen() {
    _id ++;
    while ((_id == 0) || (_idall.find(_id) != _idall.end()))
        _id++;
    return _id;
}

namespace proc {
    uint32_t add(elem_t proc, bool multi) {
        if (!multi)
            for (auto r : _all)
                if (r.proc == proc)
                    return r.id;
        _item_t r = { proc, idgen() };
        _idall.insert(r.id);
        _all.push_back(r);
        _nxt = _all.begin();
        return r.id;
    }

    bool isactive() {
        return _all.size() > 0;
    }

    bool isactive(uint32_t id) {
        return _idall.find(id) != _idall.end();
    }

    void run() {
        if (_all.empty())
            return;

        if (_nxt == _all.end())
            _nxt = _all.begin();
        auto _cur = _nxt;
        _nxt++;

        if (!(_cur->proc)()) {
            _idall.erase(_cur->id);
            _all.erase(_cur);
        }
    }
}
