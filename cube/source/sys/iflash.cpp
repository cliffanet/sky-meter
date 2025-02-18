
#include "iflash.h"
#include "stm32drv.h"
#include "cks.h"
#include "log.h"

#include <memory.h>

#if HWVER < 2

namespace iflash {
    Unlocker::Unlocker() {
        auto st = HAL_FLASH_Unlock();
        CONSOLE("FLASH_Unlock: %d", st);
        ok = st == HAL_OK;
    }

    Unlocker::~Unlocker() {
        if (!ok)
            return;
        auto st = HAL_FLASH_Lock();
        CONSOLE("HAL_FLASH_Lock: %d", st);
    }

    bool erase(int addr) {
        if (addr < _FLASH_BASE)
            return false;
        auto ea = addr - _FLASH_BASE;
        if ((ea % _FLASH_PAGE_SIZE) > 0)
            return false;

        FLASH_EraseInitTypeDef e = { 0 };
        e.TypeErase = FLASH_TYPEERASE_PAGES;
	    e.Banks     = 1;
        e.Page      = ea / _FLASH_PAGE_SIZE;
        e.NbPages   = 1;

        uint32_t err = 0;
        auto st = HAL_FLASHEx_Erase(&e, &err);

        CONSOLE("HAL_FLASHEx_Erase[0x%06x, page: %d]: %d", ea, e.Page, st);
        
        return st == HAL_OK;
    }

    bool write(int addr, const uint8_t *d, size_t sz) {
        auto a = addr;
        while (sz > 0) {
            uint8_t v[_FLASH_WBLK_SIZE];
            if (sz > _FLASH_WBLK_SIZE) {
                memcpy(v, d, _FLASH_WBLK_SIZE);
                d += _FLASH_WBLK_SIZE;
                sz -= _FLASH_WBLK_SIZE;
            }
            else {
                memcpy(v, d, sz);
                bzero(v+sz, _FLASH_WBLK_SIZE-sz);
                d += sz;
                sz = 0;
            }

            auto st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, *reinterpret_cast<_FLASH_WBLK_TYPE *>(v));
            CONSOLE("HAL_FLASH_Program[0x%06x]: %d, errno: %d", addr-_FLASH_BASE, st, st == HAL_OK ? 0 : HAL_FLASH_GetError());
            if (st != HAL_OK)
                return false;
            
            addr += _FLASH_WBLK_SIZE;
        }

        CONSOLE("saved OK on: 0x%06x", a-_FLASH_BASE);

        return true;
    }

    bool writecks(int addr, const uint8_t *d, size_t sz, size_t hdrsz) {
        uint8_t dd[sz+sizeof(cks_t)];
        memcpy(dd, d, sz);
        if ((hdrsz >= 0) && (hdrsz < sz)) {
            cks_t c(d+hdrsz, sz-hdrsz);
            memcpy(dd+sz, &c, sizeof(cks_t));
        }
        else
            bzero(dd+sz, sizeof(cks_t));

        return write(addr, dd, sizeof(dd));
    }

    bool readcks(int addr, uint8_t *d, size_t sz) {
        memcpy(d, const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(addr)), sz);
        cks_t c1(d, sz);
        cks_t c2 = *const_cast<cks_t *>(reinterpret_cast<__IO cks_t *>(addr + sz));

        return c1 == c2;
    }

    bool validcks(int addr, size_t sz) {
        cks_t c1(const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(addr)), sz);
        cks_t c2 = *const_cast<cks_t *>(reinterpret_cast<__IO cks_t *>(addr + sz));
        return c1 == c2;
    }
} // namespace iflash


#else // if HWVER < 2

namespace iflash {

    class Unlocker {
        bool ok;
        public:
            Unlocker() {
                auto st = HAL_FLASH_Unlock();
                CONSOLE("FLASH_Unlock: %d", st);
                ok = st == HAL_OK;
            }
            ~Unlocker() {
                if (!ok)
                    return;
                auto st = HAL_FLASH_Lock();
                CONSOLE("HAL_FLASH_Lock: %d", st);
            }
            operator bool() const { return ok; };
    };

    static bool _write(int addr, const uint8_t *d, size_t sz) {
        auto a = addr;
        Unlocker _l;
        if (!_l)
            return false;

        while (sz > 0) {
            uint8_t v[_FLASH_WBLK_SIZE];
            if (sz >= _FLASH_WBLK_SIZE) {
                memcpy(v, d, _FLASH_WBLK_SIZE);
                d += _FLASH_WBLK_SIZE;
                sz -= _FLASH_WBLK_SIZE;
            }
            else {
                memcpy(v, d, sz);
                bzero(v+sz, _FLASH_WBLK_SIZE-sz);
                d += sz;
                sz = 0;
            }

            auto st = _FLASH_WRITE(addr, v);
            CONSOLE("HAL_FLASH_Program[0x%06x]: %d, errno: 0x%04x", addr-_FLASH_BASE, st, st == HAL_OK ? 0 : HAL_FLASH_GetError());
            if (st != HAL_OK)
                return false;
            
            addr += _FLASH_WBLK_SIZE;
        }

        CONSOLE("saved OK on: 0x%06x", a-_FLASH_BASE);

        return true;
    }


    // Страницы используем в последовательности от самой крайней до первой.
    // т.е. если нам разрешено использовать страницы 4, 5, 6,
    // тогда сначала пишем в 6, потом в 5, потом в 4, а потом опять в 6

    #define _PAGE_VALUE(type, addr)     (*reinterpret_cast<__IO type *>(_FLASH_BASE + _a + addr))

    Page::Page() :
        _a(Page(0).next()._a)
    {}

    Page::Page(uint32_t addr) :
        _a(addr)
    {}

    Page::operator bool() const {
        return
            (_a >= _FLASH_PAGE_BEG) &&
            (_a <= _FLASH_PAGE_END+1-_FLASH_PAGE_SIZE) &&
            ((_a - _FLASH_PAGE_BEG) % _FLASH_PAGE_SIZE == 0) &&
            (_PAGE_VALUE(uint8_t, 0) == '#') &&
            (_PAGE_VALUE(uint8_t, 1) == '#') &&
            (_PAGE_VALUE(uint32_t,2) > 0) &&
            (_PAGE_VALUE(uint8_t, 6) == 0) &&
            (_PAGE_VALUE(uint8_t, 7) == 0);
    }

    uint32_t Page::num() const {
        return _PAGE_VALUE(uint32_t, 2);
    }

    Page Page::prev() const {
        uint32_t a = _a + _FLASH_PAGE_SIZE;

        // обязательно надо выравнять адрес по размеру страницы
        a -= (a-_FLASH_PAGE_BEG) % _FLASH_PAGE_SIZE;

        return Page(a);
    }

    Page Page::next() const {
        uint32_t a = _a >= _FLASH_PAGE_SIZE ? _a - _FLASH_PAGE_SIZE : 0;

        if (_a < _FLASH_PAGE_BEG) {
            // ищем самую крайнюю страницу
            a = _FLASH_PAGE_BEG;
            while (a+_FLASH_PAGE_SIZE < _FLASH_PAGE_END)
                a += _FLASH_PAGE_SIZE;
        }
        else {
            // обязательно надо выравнять адрес по размеру страницы
            a -= (a-_FLASH_PAGE_BEG) % _FLASH_PAGE_SIZE;
        }
        
        return Page(a);
    }

    bool Page::create(uint32_t num) {
        if ((_PAGE_VALUE(uint32_t, 0) != 0xffffffff) || (_PAGE_VALUE(uint32_t, 4) != 0xffffffff)) {
            Unlocker _l;
        //{
        // нужно очистить страницу

            FLASH_EraseInitTypeDef e = { 0 };
#if defined(STM32F411xE)
            e.TypeErase = FLASH_TYPEERASE_SECTORS;
            e.Sector    = (_a - _FLASH_PAGE_BEG) / _FLASH_PAGE_SIZE + _FLASH_PAGENUM_BEG;
            e.NbSectors = 1;
            e.VoltageRange = FLASH_VOLTAGE_RANGE_3;

            uint32_t err = 0;
            auto st = HAL_FLASHEx_Erase(&e, &err);

            CONSOLE("HAL_FLASHEx_Erase[0x%06x, sector: %d]: %d", _a, e.Sector, st);
#else
            e.TypeErase = FLASH_TYPEERASE_PAGES;
            e.Banks     = 1;
            e.Page      = (_a - _FLASH_PAGE_BEG) / _FLASH_PAGE_SIZE + _FLASH_PAGENUM_BEG;
            e.NbPages   = 1;

            uint32_t err = 0;
            auto st = HAL_FLASHEx_Erase(&e, &err);

            CONSOLE("HAL_FLASHEx_Erase[0x%06x, page: %d]: %d", _a, e.Page, st);
#endif // HWVER

            if (st != HAL_OK)
                return false;
        }
        
        struct __attribute__((__packed__)) {
            uint8_t h1, h2;
            uint32_t num;
            uint8_t f1, f2;
        } _p = { _FLASH_TYPE_PAGEHDR, _FLASH_TYPE_PAGEHDR, num, 0, 0 };

        return _write(_FLASH_BASE + _a, reinterpret_cast<uint8_t *>(&_p), sizeof(_p));
    }

    // формат универсальной записи:
    //      тип (1) | размер данных (1) | ... данные ... | cks (2) | выравнивание до _FLASH_WBLK_SIZE | размер данных (1)
    
    // дублирование размера в начале и конце нужно, чтобы:
    // - иметь возможность искать запись в обратную сторону
    // - дополнительный контроль валидности записи без необходимости читать всю запись и сверять cks

    #define _REC_VALUE(type, _addr)   (*reinterpret_cast<__IO type *>(_p.addr(_a + (_addr))))

    Rec::Rec() :
        _p(0),
        _a(0)
    {}

    Rec::Rec(Page page, uint32_t addr) :
        _p(page),
        _a(addr)
    {}

    Rec::operator bool() const {
        return
            _p &&
            (pgavail() > 0) && // доступное место на странице надо смотреть первым, т.к. дальше будет чтение
                            // и в случае превышение размера, будем читать либо из соседней страницы,
                            // либо вообще за пределами памяти, чтение оттуда вообще вызывает fault error
            validtype(type()) &&
            (sz() == _REC_VALUE(uint8_t, _FLASH_WBLK_ALIGN( 5 + sz() ) - 1 ));
    }

    uint8_t Rec::type() const {
        return _REC_VALUE(uint8_t, 0);
    }

    uint8_t Rec::sz() const {
        return _REC_VALUE(uint8_t, 1);
    }

    bool Rec::valid() {
        if (!*this)
            return false;
        
        cks_t c1(const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(_p.addr(_a + 2))), sz());
        cks_t c2 = *const_cast<cks_t *>(reinterpret_cast<__IO cks_t *>(_p.addr(_a + 2 + sz())));
        return c1 == c2;
    }

    bool Rec::read(uint8_t *d, size_t _sz) {
        if (!*this)
            return false;
        
        if (sz() >= _sz)
            memcpy(d, const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(_p.addr(_a + 2))), _sz);
        else {
            memcpy(d, const_cast<uint8_t *>(reinterpret_cast<__IO uint8_t *>(_p.addr(_a + 2))), sz());
            bzero(d + sz(), _sz-sz());
        }

        return true;
    }

    Rec Rec::prev() const {
        if (!*this)
            return Rec();
        
        if (_a > 0) {
            uint32_t sz = _REC_VALUE(uint8_t, -1);
            sz = _FLASH_WBLK_ALIGN(5 + sz);
            return
                _a >= sz ?
                    Rec(_p, _a-sz) :
                    Rec();
        }

        auto p = _p.prev();
        if (!p || (p.num() != _p.num() - 1))
            return Rec();

        return Rec(p, 0).pglast();
    }

    Rec Rec::next() const {
        if (!*this)
            return Rec();
        
        auto r = Rec(_p, _a + _FLASH_WBLK_ALIGN(5 + sz()));
        if (r.ispgend()) {
            auto p = _p.next();
            r = p && (p.num() == _p.num() + 1) ?
                Rec(_p.next(), 0) :
                Rec();
        }

        return r;
    }

    Rec Rec::pglast() const {
        if (!*this)
            return Rec();
        
        auto r = *this;
        while (r.pgavail() > 0) {
            auto r1 = Rec(_p, r._a + _FLASH_WBLK_ALIGN(5 + sz()));
            if (!r1 || r1.ispgend())
                break;
            r = r1;
        }

        return r;
    }

    Rec Rec::prevme() const {
        auto r = prev();

        while (r && (r.type() != type()))
            r = r.prev();
        
        return r;
    }

    Rec Rec::nextme() const {
        auto r = next();

        while (r && (r.type() != type()))
            r = r.next();
        
        return r;
    }

    static Rec _last[TYPE_SIZE];
    rtype_t rtype(uint8_t t) {
        switch (t) {
            case _FLASH_TYPE_CONFIG:
                return TYPE_CONFIG;
            case _FLASH_TYPE_LOGBOOK:
                return TYPE_LOGBOOK;
            default:
                return TYPE_ANY;
        }
    }
    void init() {
        for (auto &r: _last)
            r = Rec();
        
        for (auto r = Rec(Page(), 0); r.valid(); r = r.next()) {
            if (r.a() >= 131040) {
                CONSOLE("find: 0x%06X / 0x%08X", r.a(), r.page().addr(0));
            }
            _last[TYPE_ANY] = r;
            _last[ rtype(r.type()) ] = r;
        }
    }

    Rec last(rtype_t type) {
        return (type >= 0) && (type < TYPE_SIZE) ? _last[type] : Rec();
    }

    Rec save(uint8_t type, const uint8_t *d, size_t sz) {
        if ((sz + 5 + _FLASH_PHDR_SIZE > _FLASH_PAGE_SIZE) || (sz > 255))
            return Rec();
        
        auto r = _last[TYPE_ANY];

        if (r) {
            // вручную делаем next, оставаясь на той же странице
            auto r1 = Rec(r.page(), r.a() + r.fullsz());
            if (r1.pgavail(sz)) {
                // новые данные ещё влезут на текущую страницу
                r = r1;
            }
            else {
                // текущая страница закончилась (новые данные уже не влезут)
                if (r1.pgavail(0)) {
                    // но если место ещё осталось, его надо пометить как PAGEEND
                    uint8_t d1[_FLASH_WBLK_ALIGN(5)] = { _FLASH_TYPE_PAGEEND };
                    if (!_write(r.addr(), d1, sizeof(d1)))
                        return Rec();
                }
    
                // создаём новую страницу с номером на 1 больше
                auto p = r.page().next();
                if (!p.create( r.page().num()+1 ) || !p)
                    return Rec();
                // и начинаем писать с самого начала страницы
                r = Rec(p, 0);
            }
        }
        else {
            // если никаких валидных записей до сих пор нет,
            // то начинаем с самой крайней страницы самую первую запись
            auto p = Page(); 
            if (!p.create(1) || !p)
                return Rec();
            
            r = Rec(p, 0);
        }

        uint8_t d1[_FLASH_WBLK_ALIGN(5 + sz)];
        d1[0] = type;                        // тип
        d1[1] = sz;                          // размер данных
        memcpy(d1 + 2, d, sz);               // данные
        cks_t ck(d, sz);                    // cks
        memcpy(d1 + 2 + sz, &ck, 2);
        bzero(d1 + 4 + sz, sizeof(d1)-4-sz);  // выравнивание
        d1[sizeof(d1)-1] = sz;                // дублирование размера в конце

        // пишем
        if (!_write(r.addr(), d1, sizeof(d1)))
            return Rec();
        
        // обновляем данные _last
        _last[TYPE_ANY] = r;
        _last[ rtype(type) ] = r;
        
        return r;
    }

} // namespace iflash


#endif // if HWVER < 2
