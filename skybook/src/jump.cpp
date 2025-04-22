#include "jump.h"
#include "../../cube/source/jump/altcalc.h"

static AltBuf _abuf;
static AltJmp _jmp(false);

static AltSqBig _sq;
static AltStrict _jstr;

#include <stdio.h>

FFI_PLUGIN_EXPORT
void jump_add(float alt) {
    _abuf.tick(alt, 100);
    _jmp.tick(_abuf);
    _sq.tick(_abuf);
    _jstr.tick(_abuf);
}

FFI_PLUGIN_EXPORT
JumpInf jump_info() {
    return {
        _abuf.interval(),
        _abuf.alt(),
        _abuf.sqdiff(),

        { _abuf.avg().alt(), _abuf.avg().speed() },
        { _abuf.app().alt(), _abuf.app().speed() },
        { _abuf.sav(5).alt(), _abuf.sav(5).speed() },
        { _abuf.sav(10).alt(), _abuf.sav(10).speed() },

        {
            static_cast<int8_t>(_jmp.mode()),
            _jmp.cnt(), _jmp.tm(),
            _jmp.newcnt(),
            _jmp.newtm(),
        },

        {
            _sq.val(),
            _sq.isbig(),
            _sq.cnt(),
            _sq.tm()
        }
    };
}

FFI_PLUGIN_EXPORT
void jump_clear() {
    _abuf = AltBuf();
    _jmp.reset(AltJmp::TAKEOFF);
    _sq.reset();
    _jstr.reset();
}
