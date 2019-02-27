// Writed by yijian on 2019/2/27
#include "sys/time_thread.h"
#include "sys/log.h"
SYS_NAMESPACE_BEGIN

SINGLETON_IMPLEMENT(CTimeThread);

CTimeThread::CTimeThread()
    : _stop(false),
      _seconds(time(NULL)),
      _engine(NULL)
{
}

CTimeThread::~CTimeThread()
{
    wait();
}

time_t CTimeThread::get_seconds() const
{
#if __WORDSIZE==64
    const int64_t seconds = _seconds.operator int64_t();
#else
    const int64_t seconds = _seconds.operator int();
#endif // __WORDSIZE==64
    return static_cast<time_t>(seconds);
}

void CTimeThread::stop()
{
    _stop = true;
}

bool CTimeThread::start()
{
    try
    {
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
    const time_t startseconds = time(NULL);
    _seconds = startseconds;

    MYLOG_INFO("Time-thread start now\n");
    while (!_stop)
    {
        const time_t seconds = time(NULL);
#if __WORDSIZE==64
        _seconds = static_cast<int64_t>(seconds);
#else
        _seconds = static_cast<int>(seconds);
#endif // __WORDSIZE==64
        CUtils::millisleep(1000);
    }

    const time_t exitseconds = time(NULL);
    if (startseconds < exitseconds)
    {
        MYLOG_INFO("Time-thread exit now: %us\n", static_cast<unsigned int>(exitseconds-startseconds));
    }
    else
    {
        MYLOG_INFO("Time-thread exit now\n");
    }
}

SYS_NAMESPACE_END
