/*
    CmdTxt
*/

#ifndef _cmdtxt_H
#define _cmdtxt_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

class CmdTxt {
    public:
        typedef char str_t[64];
    
    protected:
        const char *id;

        // arg
        const str_t *_arg;
        const uint8_t _acnt;
        uint8_t _argc;

        // error / reply
        char _e[64];

    public:
        CmdTxt(const char *id, const str_t *arg, uint8_t acnt);

        bool        e()     const { return _e[0] != 0; }
        bool        ok()    const { return _e[0] == 0; }
        operator    bool()  const { return _e[0] == 0; }

    protected:
        virtual void write(const uint8_t *d, size_t sz) = 0;

        bool        argavail() const { return _argc < _acnt; };
        const char *argfetch();
        uint32_t    argnum();
        uint32_t    arghex();

        void rep(const char *s, ...);
        void err(const char *s, ...);
        void fin();
    
    public:
        template <class T, size_t _argcnt = 6>
        class Parser {
            str_t all[2 + _argcnt];
            int c = -1, i = 0;
        public:
            void read(const uint8_t *d, int sz) {
                for (; sz > 0; sz--, d++)
                    if (c < 0) {
                        if (*d == ':')
                            c = 0;
                    }
                    else 
                    if (static_cast<unsigned int>(c) < _argcnt) {
                        if ((*d >= ' ') && (*d < 128)) {
                            if (((c <= 1) && (*d == ':')) || ((c > 1) && (*d == ';'))) {
                                all[c][i] = '\0';
                                c++;
                                i = 0;
                            }
                            else
                            if (static_cast<unsigned int>(i) < (sizeof(str_t) - 1)) {
                                all[c][i] = *d;
                                i++;
                            }
                        }
                        else {
                            if (c > 0) {
                                all[c][i] = '\0';
                                T(all[0], all[1], all+2, (i > 0) || (c > 2) ? c-1 : c - 2);
                            }
                            c = -1;
                            i = 0;
                        }
                    }
            }
        };
};

#endif

#endif // _cmdtxt_H
