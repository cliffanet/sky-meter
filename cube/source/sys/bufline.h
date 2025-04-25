/*
    buf line
*/

#ifndef _bufline_H
#define _bufline_H

#ifdef __cplusplus

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

template <size_t _sz = 512>
class bufline {
    uint8_t _buf[_sz];
    uint32_t _read = 0, _str = 0;

public:
    bufline() { _buf[0] = 0; }

    uint8_t * operator*() { return _buf; }

    uint8_t * readbuf()       { return _buf+_read; }
    uint32_t  readsz()  const { return sizeof(_buf) - _read - 1; } // -1 в size - чтобы можно было поставить терминатор '\0'
    void readed(uint32_t sz) {
        auto max = readsz();
        if (sz > max)
            sz = max;
        _read += sz;
        _buf[_read] = 0;

        _str = 0;
    }

    char *tail() { return reinterpret_cast<char *>(_buf+_str); };
    // получение строки
    // если возвращён NULL - значит пора читать из файла
    char * strline() {
        if (_str >= _read) {
            // буфер закончился, пора читать новые данные.
            // хвост переносить не нужно
            _read = 0;
            return NULL;
        }
        
        auto s = tail();
        auto eol = strchr(s, '\n');
        if (eol != NULL) {
            // найдено завершение строки, возвращаем данную строку,
            // следующая строка - идёт следом после "завершения строки"
            *eol = '\0';
            _str += eol - s + 1;
            return s;
        }

        // завершение строки не найдено
        // если у нас весь буфер заполнен с самого начала, то это и есть строка
        if ((_str == 0) && (readsz() <= 0)) {
            _str = _read;   // надо сделать так, что при следующем чтении строки
                            // был возвращён NULL, а хвост не переносился
            return s;
        }

        // у нас остался обрезок строки, надо его перенести
        // в начало буфера вернуть NULL, чтобы продолжить
        // чтение файла
        if ((_str > 0) && (_read > _str)) {
            uint32_t t = _read - _str;
            for (uint32_t i = 0; i < t; i++)
                _buf[ i ] = _buf[ _str + i ];
            
            _buf[t] = 0;
            _read = t;
            _str = 0;
        }

        return NULL;
    }

};

#endif // __cplusplus

#endif // _bufline_H
