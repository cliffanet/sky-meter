#include "fshnd.h"
#include "../sys/log.h"

#include <string.h>

#include "../ff/diskio.h"

#if HWVER >= 2
#include "../sys/stm32drv.h"
extern "C" {
    void sdcard_on();
    void sdcard_off();
}
#endif

FATFS _fs = { 0 };
bool _ok;
char _path[16];

namespace fs {
    bool mount() {
        if (_ok)
            stop();

    #if HWVER >= 2
        sdcard_on();
        HAL_Delay(200);
    #endif
        strncpy(_path, "", sizeof(_path));
        _path[sizeof(_path)-1] = '\0';
    
        // mount the default drive
        FRESULT r = f_mount(&_fs, _path, 1);
        _ok = r == FR_OK;
        CONSOLE("mount: %d", r);
        if (!_ok)
            CONSOLE("f_mount(\"%s\") failed: %d", _path, r);
        
        return _ok;
    }

    bool stop() {
        if (!_ok)
            return false;

        FRESULT r = f_mount(NULL, _path, 0);
        if (r != FR_OK)
            CONSOLE("Unmount failed, res = %d", r);
        
        CONSOLE("unmount: %d", r);
    #if HWVER >= 2
        sdcard_off();
    #endif

        return true;
    }

    const FATFS &inf() {
        return _fs;
    }

    ctype_t ctype() {
        BYTE ct = 0;
        disk_ioctl(0, MMC_GET_TYPE, &ct);
        return
            ct ==   0           ? CT_NONE :
            ct ==   CT_MMC3     ? CT_MMC3 :
            ct ==   CT_MMC4     ? CT_MMC4 :
            ct &    CT_MMC      ? CT_MMC3 :
            ct ==   CT_SDC1     ? CT_SDC1 :
            ct ==   CT_SDC2     ? CT_SDC2 :
            ct ==   CT_SDC2BL   ? CT_SDC2BL :
            ct &    CT_SDC      ? CT_SDC :
            ct &    CT_BLOCK    ? CT_BLOCK :
                                  CT_UNKNOWN;
    }

    File::File() :
        _ok(false)
    {}

    File::File(const TCHAR *path, BYTE mode)
    {
        open(path, mode);
    }

File::~File() {
    if (!_ok)
        return;
    F(f_close(&_fh));
}

bool File::open(const TCHAR *path, BYTE mode) {
    CONSOLE("open file: %s", path);
    FR(f_open(&_fh, path, mode), _ok=false; return false);
    _ok = true;
    return true;
}

bool File::close() {
    _ok = false;
    FR(f_close(&_fh), return false);
    return true;
}

int32_t File::read(void *buff, UINT btr) {
    UINT szr;
    
    FR(f_read(&_fh, buff, btr, &szr), return -1);
    return szr;
}

bool File::write(const void *buff, UINT btw) {
    UINT szw;
    
    FR(f_write(&_fh, buff, btw, &szw), return false);
    if (btw != szw) {
        CONSOLE("file write fail: %u => %u", btw, szw);
        return false;
    }

    return true;
}

} // namespace fs
