#include "jump.h"
#include "../../cube/source/jump/altcalc.h"

static AltBuf _abuf;
static AltJmp _jmp(true);

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
        { _abuf.sav().alt(), _abuf.sav().speed() },

        {
            static_cast<int8_t>(_jmp.mode()),
            _jmp.cnt(), _jmp.tm(),
            _jmp.mode() == AltJmp::TAKEOFF ? _jmp.ff().cnt() : _jmp.newcnt(),
            _jmp.mode() == AltJmp::TAKEOFF ? _jmp.ff().tm() : _jmp.newtm(),
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
