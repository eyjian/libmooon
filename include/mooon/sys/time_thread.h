// Writed by yijian on 2019/2/27
#ifndef MOOON_SYS_TIME_THREAD_H
#define MOOON_SYS_TIME_THREAD_H
#include <mooon/sys/atomic.h>
#include <mooon/sys/thread_engine.h>
#include <mooon/sys/utils.h>
#include <mooon/utils/config.h>
#include <time.h>
SYS_NAMESPACE_BEGIN

// 提供秒级时间，
// 可用于避免多个线程重复调用time(NULL)
class CTimeThread
{
    SINGLETON_DECLARE(CTimeThread);

public:
    CTimeThread();
    ~CTimeThread();
    time_t get_seconds() const;
    void stop(); // 同一对象在stop后不能重复使用
    bool start();
    void wait();
    void run();

private:
    CAtomic<bool> _stop;
#if __WORDSIZE==64
    CAtomic<int64_t> _seconds;
#else
    CAtomic<int> _seconds;
#endif // __WORDSIZE==64
    CThreadEngine* _engine;
};

SYS_NAMESPACE_END
#endif // MOOON_SYS_TIME_THREAD_H
