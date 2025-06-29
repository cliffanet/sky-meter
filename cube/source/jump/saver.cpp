#include "saver.h"
#include "../sys/log.h"
#include "../sdcard/fshnd.h"
#include "../jump/proc.h"
#include "../jump/altcalc.h"
#include "../view/text.h"
#include "../sys/maincfg.h"
#include "../sys/clock.h"
#include "../sys/sproc.h"

#include <string>
#include <string.h>

#include "../ff/diskio.h"

static SPRC_VAR;
static fs::File _f;
static enum _sop_t {
    OP_NULL = 0,
    OP_FULL,
    OP_TRACEMAN
} _op = OP_NULL;

#ifdef USE_JMPTRACE
static jmp::log_ring_t::snapcursor _log;
static unsigned int _log_sz = 0, _log_cnt = 0;
#endif // USE_JMPTRACE

#ifdef USE_CONSOLE
static logbuf_iterator_t _logit;
static const char *_logbuf = NULL;
#endif // USE_CONSOLE


static bool _saver() {
    SPROC
        CONSOLE("Saving to sd-card by op: %d", _op);

        if (!fs::ismount()) {
            _op = OP_NULL;
            return false;
        }
        _f.close();
    
#ifdef USE_LOGBOOK
    SPRC_BRK

        if (_op == OP_FULL) {
            CONSOLE("Writing to " SDCARD_LOGBOOK "...");

            FILINFO fi;
            bool ex = f_stat(SDCARD_LOGBOOK, &fi) == FR_OK;

            _f.open(SDCARD_LOGBOOK, FA_OPEN_APPEND | FA_WRITE);
            if (_f && !ex) {
                char s[128];
                auto sz = snprintf(s, sizeof(s), 
                    "\"Date/time\",\"Jump number\",\"Takeoff time\","
                    "\"Begin alt\",\"FreeFall time\",\"Canopy alt\",\"Canopy time\"\n");
                if (!_f.write(s, sz))
                    _f.close();
            }
        }
    
    SPRC_BRK

        if (_f) {
            auto _l = jmp::last();
            char s[128];
            auto sz = jsave::logbook2csv(s, sizeof(s), _l);
            _f.write(s, sz);
            _f.close();
        }
#endif // USE_LOGBOOK
    
#ifdef USE_JMPTRACE
    SPRC_BRK

        char fname[64];
        if (_op == OP_FULL) {
            auto _l = jmp::last();
            const auto &tm = _l.tm;

            snprintf(fname, sizeof(fname), "jump_%lu_%u-%02u-%04u_%u-%02u.csv", _l.num, tm.day, tm.mon, tm.year, tm.h, tm.m);
            _f.open(fname, FA_CREATE_ALWAYS | FA_WRITE);
        }
        else
        if (_op == OP_TRACEMAN) {
            auto tm = tmNow();

            snprintf(fname, sizeof(fname), "jump_%lu_%u-%02u-%04u_%u-%02u-%02d_man.csv", cfg->jmpcnt, tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s);
            _f.open(fname, FA_CREATE_ALWAYS | FA_WRITE);
        }
    
    SPRC_BRK

        if (_f) {
            char s[128];
            
            auto sz = snprintf(s, sizeof(s), 
                "\"alt\",\"calculated change\",\"factical mode\"\n");
            if (!_f.write(s, sz))
                _f.close();
            
            _log = jmp::trace().snapbeg();
            _log_sz = sz, _log_cnt = 0;
        }
    
    SPRC_BRK

        if (_f) {
            auto &l = *_log;
            
            char mclc[10], mchg[10];
            if (l.mclc)
                snprintf(mclc, sizeof(mclc), "\"%c\"", l.mclc);
            else
                mclc[0] = '\0';
            if (l.mchg)
                snprintf(mchg, sizeof(mchg), "\"%c\"", l.mchg);
            else
                mchg[0] = '\0';
            
            char s[32];
            auto sz = snprintf(s, sizeof(s), "%d,%s,%s\n", l.alt, mclc, mchg);
            if (!_f.write(s, sz))
                _f.close();

            _log_sz += sz;
            _log_cnt ++;
            
            ++_log;
            if (_log.availr() > 0)
                return true;
        }
    
    SPRC_BRK
    
        if (_f) {
            CONSOLE("writed: %d lines, %d bytes", _log_cnt, _log_sz);
            _f.close();
        }
        
#endif // USE_JMPTRACE
        
#ifdef USE_CONSOLE
    SPRC_BRK

        CONSOLE("Writing to " SDCARD_SYSLOG "...");

        _f.open(SDCARD_SYSLOG, FA_OPEN_APPEND | FA_WRITE);
        if (_f) {
            const char s[] = "--------------------------------------\n";
            if (!_f.write(s, sizeof(s)-1))
                _f.close();
        }
        if (_f)
            _logbuf = logbufbeg(&_logit);

    SPRC_BRK

        if (_f && (_logbuf != NULL)) {
            if (_f.write(_logbuf, strlen(_logbuf)))
                _logbuf = logbufnxt(&_logit);
            else
                _f.close();
        }
        if (_f && (_logbuf != NULL))
            return true;
        
        _f.close();

#endif // USE_CONSOLE
        
    SPRC_END

    if (_f) {
        CONSOLE("Not closed file");
        _f.close();
    }
    fs::stop();
    _op = OP_NULL;

    return false;
}

static void _saverbeg(_sop_t o) {
    if (!fs::mount())
        return;
    SPRC_INIT;
    _op = o;
    proc::add(_saver);
}

namespace jsave {
    bool isactive() {
        return (_op > OP_NULL) && fs::ismount();
    }

    int logbook2csv(char *s, size_t sz, LogBook::item_t &_l) {

        const auto &tm = _l.tm;
        return snprintf(s, sz, 
            "\"%2u.%02u.%04u %2u:%02u:%02u\",%lu,\"" TXT_LOGBOOK_MINSEC "\","
            "%u,%u,%u,\"" TXT_LOGBOOK_MINSEC "\"\n",
            tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s,
            _l.num,
            _l.toffsec / 60, _l.toffsec % 60,
            _l.begalt, _l.ffsec,
            _l.cnpalt,
            _l.cnpsec / 60, _l.cnpsec % 60
        );
    }

#ifdef USE_JMPTRACE
    void trace() {
        _saverbeg(OP_TRACEMAN);
    }
#endif // USE_JMPTRACE

    void full() {
        _saverbeg(OP_FULL);
    }

}
