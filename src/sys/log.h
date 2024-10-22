/*
    Log
*/

#ifndef _core_log_H
#define _core_log_H

#include "../../def.h"

#ifdef __cplusplus
 extern "C" {
#endif


#ifdef FWVER_DEBUG

const char * __extrfname(const char * path);
#define LOG_FMT(s)        "[%s:%u] %s(): " s, __extrfname(__FILE__), __LINE__, __FUNCTION__

void conslog(const char *s, ...);
#define CONSOLE(txt, ...) conslog(LOG_FMT(txt), ##__VA_ARGS__)

#else // FWVER_DEBUG

#define CONSOLE(txt, ...)

#endif // FWVER_DEBUG

#ifdef __cplusplus
}
#endif

#endif // _core_log_H
