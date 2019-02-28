// Writed by yijian on 2019/2/27
#include "sys/time_thread.h"
#include "sys/log.h"
#include <sys/time.h>
SYS_NAMESPACE_BEGIN

SINGLETON_IMPLEMENT(CTimeThread);

CTimeThread::CTimeThread()
    : _stop(false),
      _interval_milliseconds(1000),
      _engine(NULL)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

#if __WORDSIZE==64
    _seconds = static_cast<int64_t>(tv.tv_sec);
    _milliseconds = static_cast<int64_t>(tv.tv_usec);
#else
    _seconds = static_cast<int>(tv.tv_sec);
    _milliseconds = static_cast<int>(tv.tv_usec);
#endif // __WORDSIZE==64
}

CTimeThread::~CTimeThread()
{
    wait();
}

int64_t CTimeThread::get_seconds() const
{
#if __WORDSIZE==64
    return _seconds.operator int64_t();
#else
    return _seconds.operator int();
#endif // __WORDSIZE==64
}

int64_t CTimeThread::get_milliseconds() const
{
#if __WORDSIZE==64
    return _milliseconds.operator int64_t();
#else
    return _milliseconds.operator int();
#endif // __WORDSIZE==64
}

void CTimeThread::stop()
{
    _stop = true;
}

bool CTimeThread::start(uint32_t interval_milliseconds)
{
    try
    {
        _interval_milliseconds = interval_milliseconds;
        _engine = new mooon::sys::CThreadEngine(mooon::sys::bind(&CTimeThread::run, this));
        return true;
    }
    catch (sys::CSyscallException& ex)
    {
        MYLOG_ERROR("Start time-thread failed: %s\n", ex.str().c_str());
        return false;
    }
}

void CTimeThread::wait()
{
    if (_engine != NULL)
    {
        _engine->join();
        delete _engine;
        _engine = NULL;
    }
}

void CTimeThread::run()
{
    struct timeval start_tv, exit_tv;
    gettimeofday(&start_tv, NULL);

    MYLOG_INFO("Time-thread start now\n");
    while (!_stop)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        const int64_t milliseconds = static_cast<int64_t>(tv.tv_sec*1000 + tv.tv_usec/1000);
#if __WORDSIZE==64
        _seconds = static_cast<int64_t>(tv.tv_sec);
        _milliseconds = milliseconds;
#else
        _seconds = static_cast<int>(tv.tv_sec);
        _milliseconds = static_cast<int>(milliseconds);
#endif // __WORDSIZE==64

        CUtils::millisleep(_interval_milliseconds);
    }

    gettimeofday(&exit_tv, NULL);
    if (start_tv.tv_sec<=exit_tv.tv_sec ||
        start_tv.tv_usec<=exit_tv.tv_usec)
    {
        const uint64_t interval_milliseconds = (exit_tv.tv_sec-start_tv.tv_sec)*1000 + (exit_tv.tv_usec-start_tv.tv_usec)/1000;
        MYLOG_INFO("Time-thread exit now: %" PRIu64"ms\n", interval_milliseconds);
    }
    else
    {
        MYLOG_INFO("Time-thread exit now\n");
    }
}

SYS_NAMESPACE_END
