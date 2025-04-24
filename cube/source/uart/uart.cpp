
#include "uart.h"
#include "cmdtxt.h"
#include "../sys/log.h"

#include <cstring>

#include "../sys/stm32drv.h"
#include "usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t _active = 0;

class _cmd : public CmdTxt {
    void write(const uint8_t *d, size_t sz) {
        if (sz == 0)
            return;
        CDC_Transmit_FS(const_cast<uint8_t *>(d), sz);
        // надо дождаться окончания передачи, т.к. она ассинхронная, а буфер str уничтожается при выходе
        USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
        for (volatile int i = 0; (i < 1000000) && (hcdc->TxState != 0); i++) asm("");
    }

    // cmd
    void _echo() {
        _active = 50;
    }
    void _term() {
        _active = 0;
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
