/*
    Worker
*/

#ifndef _sys_worker_H
#define _sys_worker_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus

/*
    Это облегчённая версия воркера - без использования shared_ptr.
    Создаётся/останавливается каждый воркер - через стандартные
    new/delete для класса воркера.
    Контроллер не нужен, т.к. есть прямой доступ к классу воркера.

    Единственная проблема такой версии - невозможность самоуничтожиться
    воркеру по окончанию его работы.

    Для этого можно использовать флаг O_AUTODEL, при котором
    wrkProcess сам удалит объект при завершении работы.
    Но снаружи, если мы сохранили ссылку на объект,
    мы можем получить ошибку обращения к памяти.
*/

/* ------------------------------------------------------------------------------------------- *
 *  Базовый класс для каждого воркера
 * ------------------------------------------------------------------------------------------- */
class Wrk {
    public:
        // статусы выхода из выполнения воркера
        typedef enum {
            DLY,          // в этом цикле больше не надо выполнять этот воркер
            RUN,          // этот процесс надо выполнять и дальше, если ещё есть время на это
            END,          // завершить воркер
        } state_t;

        Wrk();
        virtual ~Wrk();

        typedef void * key_t;

        virtual void timer() {}
        virtual state_t run() = 0;
        virtual void end() {};

        typedef enum {
            O_INLIST = 0,       // процесс находится в списке воркеров
            O_NEEDWAIT,         // не выполнять это воркер в текущем цикле обработки wrkProcess() - воркер что-то ждёт
            O_NEEDFINISH,       // воркер надо завершить (выполнить end()) и больше не выполнять
            O_FINISHED,         // завершённый воркер - end() уже выполнен и этот воркер больше не надо выполнять совсем
            O_AUTODELETE        // надо сделать delete по завершению
        } opts_t;
        
        uint16_t    opts()              const { return __opts; }
        bool        opt(opts_t fl)      const { return (__opts & (1 << fl)) > 0; }
        void        optclr()            { __opts = 0; };
        void        optclr(opts_t fl)   { __opts = (1 << fl); };
        void        optset(opts_t fl)   { __opts |= (1 << fl); }
        void        optdel(opts_t fl)   { __opts &= ~static_cast<uint16_t>(1 << fl); }
    
    protected:
        uint32_t __line = 0;
    private:
        uint16_t __opts = 0;
};

// воркер, возвращающий значение
template <class T>
class WrkRet : public Wrk {
    public:
        const T &ret() const { return ret; }
    private:
        T m_ret;
};

// воркер, возвращающий bool: isok()
class WrkOk : public Wrk {
    public:
        bool isok() const { return m_isok; }
    protected:
        state_t ok() {
            m_isok=true;
            return END;
        }
        state_t err() {
            m_isok=false;
            return END;
        }
        bool m_isok = false;
};

// воркер процессинг, за которым можно
// наблюдать процент выполненного, а так же,
// получать статус isok()
class WrkCmpl : public WrkOk {
    public:
        typedef struct {
            uint32_t val, sz;
        } cmpl_t;

        WrkCmpl() :
            m_cmpl({ 0, 0 })
            {}

        const cmpl_t& cmpl() const { return m_cmpl; };

    protected:
        cmpl_t m_cmpl;
};

#define WPROC \
            switch (__line) { \
                case 0: {

#define WPRC_BREAK              __line = __LINE__; }                case __LINE__: {
#define WPRC_RUN                __line = __LINE__; return RUN; }    case __LINE__: {
#define WPRC_DLY                __line = __LINE__; return DLY; }    case __LINE__: {
#define WPRC(ret) \
                    return ret; \
                } \
            } \
            \
            CONSOLE("Worker line fail: %d", __line); \
            return END;

#define WPRC_AWAIT(v, T, ...) \
                v = wrkRun<T>(__VA_ARGS__); \
                WPRC_BREAK \
                if (v.isrun()) return DLY;



bool wrkEmpty();

 extern "C" {
#endif

// исполнение всех существующих воркеров в течение времени tmmax (мс)
uint32_t wrkProcess(uint32_t msmax);

#ifdef __cplusplus
}
#endif


#endif // _sys_worker_H
