
#include "uart.h"
#include "cmdtxt.h"
#include "../sys/log.h"
#include "../sys/iflash.h"
#include "../jump/saver.h"

#include "../sdcard/fshnd.h"
#include "../ff/diskio.h"
#include "../sys/bufline.h"

#include <cstring>

#include "../sys/stm32drv.h"
#include "usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t _active = 0;

class _cmd : public CmdTxt {
    void write(const uint8_t *d, size_t sz) {
        USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
        // при неподключенном usb tx может вызывать
        // USBD_BUSY, в результате мы будем ждать
        // несуществующую отправку.
        if (hcdc->TxState != 0)
            return;
        if (sz == 0)
            return;
        CDC_Transmit_FS(const_cast<uint8_t *>(d), sz);
        // надо дождаться окончания передачи, т.к. она ассинхронная, а буфер str уничтожается при выходе
        for (volatile int i = 0; (i < 1000000) && (hcdc->TxState != 0); i++) asm("");
    }

    // cmd
    void _echo() {
        _active = 50;
    }
    void _term() {
        _active = 0;
    }

    void _logbook() {
        auto cnt = argavail() ? argnum() : 50;
        LogBook::item_t l;
        char s[128];

        for (auto r = iflash::last(iflash::TYPE_LOGBOOK); r && (cnt > 0); r = r.prevme(), cnt--)
            if (r.read(l)) {
                auto n = jsave::logbook2csv(s, sizeof(s), l);
                s[n-1] = '\0'; // убираем терминирующий \n
                rep("%s", s);
            }
    }

    void _tracelist() {
        fs::mounter _mnt;
        if (!_mnt) {
            CONSOLE("sdcard fail");
            return;
        }
        
        DIR dh;
        FR(f_opendir(&dh, "/"), return);

        FILINFO f;
        while (f_readdir(&dh, &f) == FR_OK) {
            if (f.fname[0] == '\0')
                break;
            if (f.fattrib & AM_DIR)
                continue;
            auto l = strlen(f.fname);
            if (
                    (l < 10) ||
                    (strncmp(f.fname, "jump_", 5) != 0) ||
                    (strncmp(f.fname + l - 4, ".csv", 4) != 0) ||
                    (f.fname[5] < '0') ||
                    (f.fname[5] > '9')
                )
                continue;
            
            rep("%s", f.fname);
        }

        F(f_closedir(&dh))
    }

    void _traceget() {
        auto fname = argfetch();
        if (fname == NULL)
            return;
        
        fs::mounter _mnt;
        if (!_mnt) {
            CONSOLE("sdcard fail");
            return;
        }
        
        fs::File fh(fname);
        if (!fh)
            return;
        
        bufline<> b;
        for (;;) {
            // -1 в size - чтобы можно было поставить терминатор '\0'
            auto sz = fh.read(b.readbuf(), b.readsz());
            if (sz <= 0)
                break;
            b.readed(sz);

            const char *s;
            while ((s = b.strline()) != NULL)
                rep("%s", s);
        }

        auto t = b.tail();
        if (*t)
            rep("%s", t);
    }
public:
    _cmd(const char *id, const char *name, const str_t *arg, uint8_t acnt) :
        CmdTxt(id, arg, acnt)
    {
//#ifdef FWVER_DEBUG
        char par[256], *s = par;
        auto l = sizeof(par);
        *s = '\0';
        for (int i = 0; i < acnt; i++) {
            auto x = snprintf(s, l, "%s", arg[i]);
            s += x;
            l -= x;
            if ((i < acnt-1) && (l > 0)) {
                *s = ';';
                s++;
                l--;
            }
        }
        CONSOLE("cmd[%s]: %s(%s)", id, name, par);
//#endif
#define _cc(cmd)    if (strcasecmp(name, #cmd) == 0) _ ## cmd(); else

        _cc(echo)
        _cc(term)
        _cc(logbook)
        _cc(tracelist)
        _cc(traceget)
        err("Unknown cmd");

#undef _cc

        fin();
    }
};

static CmdTxt::Parser<_cmd> _p;
static uint8_t _buf[1024];
static int _b = 0;

namespace uart {
    bool isactive() {
        return _active > 0;
    }

    void tick() {
        if (_active > 0)
            _active --;
        
        if (_b == 0)
            return;
        
        _p.read(_buf, _b);
        _b = 0;
    }
}

extern "C"
void uartrecv(const uint8_t *data, uint32_t sz) {
    // Этот блок выполняется по прерыванию от usb
    // и плохо переваривает, если в нём обратно печатать в usb,
    // поэтому без промежуточного буфера никак.
    // Тут только заполняем промежуточный буфер.
    auto bs = sizeof(_buf) - _b;
    if (bs == 0)
        return;
    if (sz > bs)
        sz = bs;
    memcpy(_buf+_b, data, sz);
    _b += sz;
}
