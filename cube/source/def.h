/*
    common
*/

#ifndef _def_H
#define _def_H

#define STRINGIFY(x)        #x
#define TOSTRING(x)         STRINGIFY(x)
#define PPCAT_NX(A, B)      A ## B

#define vmap(value, low1, high1, low2, high2) \
                            (low2 + (value - low1) * (high2 - low2) / (high1 - low1))

#ifndef HWVER
#define HWVER               2
#endif

#ifndef FWVER_NUM
#define FWVER_NUM           .dev
#define FWVER_DEBUG
#endif

#ifndef FWVER_LANG
#define FWVER_LANG          'R'
#endif

/* FW version name */
#ifdef FWVER_NUM

#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
#define FWVER_LANG_NAME     ".ru"
#else
#define FWVER_LANG_NAME     ".en"
#endif

#define FWVER_NAME          TOSTRING(FWVER_NUM) ".hw" TOSTRING(HWVER) FWVER_LANG_NAME

#endif

#define USE_CONSOLE
#define USE_DEVMENU
#define USE_MENU
#define USE_JMPINFO
#define USE_JMPTRACE
#define USE_LOGBOOK

#endif // _def_H
