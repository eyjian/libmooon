// Writed by yijian on 2019/1/25
#ifndef MOOON_SYS_THREAD_BASE_H
#define MOOON_SYS_THREAD_BASE_H
#include "mooon/sys/atomic.h"
#include "mooon/sys/utils.h"
SYS_NAMESPACE_BEGIN

// The base class for thread
//
// #include <chrono>
// #include <system_error>
// class CMyThread: public CThreadBase
// {
// public:
//     void run()
//     {
//         while (!to_stop())
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//         }
//     }
// };
class CThreadBase
{
public:
    CThreadBase()
        : _stop(false)
    {
    }

    void stop()
    {
        _stop = true;
    }

    bool to_stop() const
    {
        return _stop;
    }

protected:
    CAtomic<bool> _stop;
};

SYS_NAMESPACE_END
#endif // MOOON_SYS_THREAD_BASE_H
