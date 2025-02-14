#include "fshnd.h"
#include "../sys/log.h"

#include <string.h>

#include "../ff/diskio.h"

FSMount::FSMount(const char *path) {
    strncpy(_path, path, sizeof(_path));
    _path[sizeof(_path)-1] = '\0';

    // mount the default drive
    FRESULT r = f_mount(&_fs, _path, 1);
    _ok = r == FR_OK;
    CONSOLE("mount: %d", r);
    if (!_ok)
        CONSOLE("f_mount(\"%s\") failed: %d", _path, r);
}

FSMount::~FSMount() {
    FRESULT r = f_mount(NULL, _path, 0);
    if (r != FR_OK)
        CONSOLE("Unmount failed, res = %d", r);
    
    CONSOLE("unmount: %d", r);
}

FSMount::ctype_t FSMount::ctype() {
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

FSFile::FSFile(const TCHAR *path, BYTE mode) {
    CONSOLE("open file: %s", path);
    FR(f_open(&_fh, path, mode), _ok=false; return);
    _ok = true;
}

FSFile::~FSFile() {
    if (!_ok)
        return;
    F(f_close(&_fh));
}

bool FSFile::write(const void *buff, UINT btw) {
    UINT szw;
    
    FR(f_write(&_fh, buff, btw, &szw), return false);
    if (btw != szw) {
        CONSOLE("file write fail: %u => %u", btw, szw);
        return false;
    }

    return true;
}
