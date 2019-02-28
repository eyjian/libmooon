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
// 注：32位平台上的毫秒级不准
class CTimeThread
{
    SINGLETON_DECLARE(CTimeThread);

public:
    CTimeThread();
    ~CTimeThread();
    int64_t get_seconds() const;
    int64_t get_milliseconds() const;
    void stop(); // 同一对象在stop后不能重复使用
    bool start(uint32_t interval_milliseconds);
    void wait();
    void run();

private:
    CAtomic<bool> _stop;
#if __WORDSIZE==64
    CAtomic<int64_t> _seconds;
    CAtomic<int64_t> _milliseconds;
#else
    CAtomic<int> _seconds;
    CAtomic<int> _milliseconds;
#endif // __WORDSIZE==64
    uint32_t _interval_milliseconds;
    CThreadEngine* _engine;
};

SYS_NAMESPACE_END
#endif // MOOON_SYS_TIME_THREAD_H
