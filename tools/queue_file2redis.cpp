// Writed by yijian on 2021/5/20
// Load from file to redis list
#include <mooon/sys/main_template.h>
#include <mooon/sys/mmap.h>
#include <mooon/sys/safe_logger.h>
#include <mooon/sys/signal_handler.h>
#include <mooon/sys/stop_watch.h>
#include <mooon/sys/utils.h>
#include <mooon/utils/args_parser.h>
#include <mooon/utils/print_color.h>
#include <mooon/utils/string_utils.h>
#include <r3c/r3c.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <sstream>
#include <system_error>
#include <thread>
#include <vector>

// 可选的用来标识进程的字符串
STRING_ARG_DEFINE(label, "", "Optional string used to identify process.");

// Redis (source)
STRING_ARG_DEFINE(file, "", "File path to load.");

// Redis (target)
STRING_ARG_DEFINE(redis_nodes, "", "Nodes list of redis.");
STRING_ARG_DEFINE(redis_password, "", "Password of redis.");
STRING_ARG_DEFINE(redis_key_prefix, "", "Key prefix of redis list.");
INTEGER_ARG_DEFINE(int, redis_key_count, 1, 1, 2020, "Number of redis list keys with the given prefix.");
INTEGER_ARG_DEFINE(int, redis_timeout, 60000, 0, 86400000, "Timeout to lpush redis in millisecond.");

// 批量数，即一次批量加载多少
INTEGER_ARG_DEFINE(int, batch, 1, 1, 100000, "Batch to move from redis to kafka.");

// metric 统计间隔（单位：秒）
INTEGER_ARG_DEFINE(int, interval, 10, 0, 3600, "Interval to count metric in seconds.");

// 分割符合
STRING_ARG_DEFINE(delimiter, "\n", "Single character separator of logs.");

class CFile2redisLoader;

struct Metric
{
    std::atomic<uint32_t> push_number;
    std::atomic<uint32_t> redis_exception_number;
} metric;

class CFile2redis: public mooon::sys::CMainHelper
{
private:
    virtual bool on_check_parameter();
    virtual bool on_init(int argc, char* argv[]);
    virtual bool on_run();
    virtual void on_fini();
    virtual void on_terminated();

private:
    bool init_metric_logger();
    bool start_file2redis_movers();
    void stop_file2redis_movers();
    void wait_file2redis_movers();

private:
    std::shared_ptr<mooon::sys::CSafeLogger> _metric_logger;
    std::vector<std::shared_ptr<CFile2redisLoader>> _file2redis_loaders;
};

class CFile2redisLoader
{
public:
    CFile2redisLoader(CFile2redis* redis2redis);
    ~CFile2redisLoader();
    bool start(time_t now, int index);
    void stop();
    void wait();

private:
    bool init_source_file();
    bool init_target_redis();
    void run();

private:
    bool get_logs(int batch, std::vector<std::string>* logs);
    bool lpush_logs(const std::vector<std::string>& logs);
    std::string get_target_redis_key();

private:
    int _index;
    CFile2redis* _file2redis;
    std::atomic<bool> _stop;
    std::shared_ptr<std::thread> _load_thread;
    std::shared_ptr<r3c::CRedisClient> _redis;
    std::default_random_engine _random_engine;
    mooon::sys::mmap_t* _file_ptr;
    size_t _file_offset;
};

extern "C"
int main(int argc, char* argv[])
{
    CFile2redis f2r;
    return mooon::sys::main_template(&f2r, argc, argv, NULL);
}

//
// CRedis2redis
//

bool CFile2redis::on_check_parameter()
{
    // --file
    if (mooon::argument::file->value().empty())
    {
        fprintf(stderr, "Parameter[--file] is not set\n");
        return false;
    }
    // --redis_nodes
    if (mooon::argument::redis_nodes->value().empty())
    {
        fprintf(stderr, "Parameter[--redis_nodes] is not set\n");
        return false;
    }
    // --redis_key_prefix
    if (mooon::argument::redis_key_prefix->value().empty())
    {
        fprintf(stderr, "Parameter[--redis_key_prefix] is not set\n");
        return false;
    }
    // --delimiter
    if (mooon::argument::delimiter->value().empty())
    {
        fprintf(stderr, "Parameter[--delimiter] is not set\n");
        return false;
    }
    if (mooon::argument::delimiter->value().size() != 1)
    {
        fprintf(stderr, "Parameter[--delimiter] with a invalid value\n");
        return false;
    }
    if (!mooon::argument::label->value().empty())
    {
        this->set_logger(mooon::argument::label->value());
    }
    return true;
}

bool CFile2redis::on_init(int argc, char* argv[])
{
    if (!init_metric_logger())
    {
        return false;
    }
    else
    {
        return start_file2redis_movers();
    }
}

bool CFile2redis::on_run()
{
    const int interval = mooon::argument::interval->value();

    MYLOG_INFO("Redis2redis is started now\n");
    if (interval <= 0)
    {
        wait_file2redis_movers();
    }
    else
    {
        while (!this->to_stop())
        {
            for (int i=0; i<interval&&!this->to_stop(); ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            if (metric.push_number > 0)
            {
                const uint32_t push_number = metric.push_number.exchange(0);
                const uint32_t redis_exception_number = metric.redis_exception_number.exchange(0);
                _metric_logger->log_raw("push:%u,redis:%u\n", push_number, redis_exception_number);
            }
        }
        wait_file2redis_movers();
    }
    return true;
}

void CFile2redis::on_fini()
{
    stop_file2redis_movers();
    wait_file2redis_movers();
}

void CFile2redis::on_terminated()
{
    mooon::sys::CMainHelper::on_terminated();
    stop_file2redis_movers();
}

bool CFile2redis::init_metric_logger()
{
    try
    {
        std::string suffix;
        if (mooon::argument::label->value().empty())
            suffix = "metric";
        else
            suffix = mooon::argument::label->value() + std::string("_metric");
        _metric_logger.reset(mooon::sys::create_safe_logger(true, mooon::SIZE_1K, suffix));
        _metric_logger->enable_raw_log(true, true);
        return true;
    }
    catch (mooon::sys::CSyscallException& ex)
    {
        MYLOG_ERROR("Create metric logger failed: %s\n", ex.str().c_str());
        return false;
    }
}

bool CFile2redis::start_file2redis_movers()
{
    const time_t now = time(NULL);
    for (int i=0; i<1; ++i)
    {
        std::shared_ptr<CFile2redisLoader> redis2redis_mover(new CFile2redisLoader(this));
        if (redis2redis_mover->start(now, i))
        {
            _file2redis_loaders.push_back(redis2redis_mover);
        }
        else
        {
            stop_file2redis_movers();
            break;
        }
    }
    return true;
}

void CFile2redis::stop_file2redis_movers()
{
    for (auto redis2redis_mover: _file2redis_loaders)
    {
        if (redis2redis_mover.get() != nullptr)
            redis2redis_mover->stop();
    }
}

void CFile2redis::wait_file2redis_movers()
{
    for (auto redis2redis_mover: _file2redis_loaders)
    {
        if (redis2redis_mover.get() != nullptr)
            redis2redis_mover->wait();
    }
    _file2redis_loaders.clear();
}

//
// CRedis2redisMover
//

CFile2redisLoader::CFile2redisLoader(CFile2redis* redis2redis)
    : _index(-1), _file2redis(redis2redis), _stop(false),
      _file_ptr(NULL), _file_offset(0)
{
}

CFile2redisLoader::~CFile2redisLoader()
{
    if (_file_ptr != NULL)
    {
        mooon::sys::CMMap::unmap(_file_ptr);
        _file_ptr = NULL;
    }
}

bool CFile2redisLoader::start(time_t now, int index)
{
    try
    {
        _index = index;

        if (!init_source_file() || !init_target_redis())
        {
            return false;
        }
        else
        {
            _load_thread.reset(new std::thread(&CFile2redisLoader::run, this));
            _random_engine.seed(now+index);
            return true;
        }
    }
    catch (std::system_error& ex)
    {
        MYLOG_ERROR("Start load thread(%d) failed: %s\n", index, ex.what());
        return false;
    }
}

void CFile2redisLoader::stop()
{
    _stop = true;
}

void CFile2redisLoader::wait()
{
    if (_load_thread.get() != nullptr && _load_thread->joinable())
    {
        _load_thread->join();
        _load_thread.reset();
    }
}

bool CFile2redisLoader::init_source_file()
{
    try
    {
        _file_ptr = mooon::sys::CMMap::map_read(mooon::argument::file->c_value());
        return true;
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("mmap file://%s failed: %s.\n", mooon::argument::file->c_value(), ex.str().c_str());
        return false;
    }
}

bool CFile2redisLoader::init_target_redis()
{
    try
    {
        _redis.reset(
                new r3c::CRedisClient(
                        mooon::argument::redis_nodes->value(),
                        mooon::argument::redis_password->value(),
                        mooon::argument::redis_timeout->value(),
                        mooon::argument::redis_timeout->value()));
        return true;
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Create RedisClient by %s failed: %s.\n", mooon::argument::redis_nodes->c_value(), ex.str().c_str());
        return false;
    }
}

void CFile2redisLoader::run()
{
    const int batch = mooon::argument::batch->value();

    while (!_stop)
    {
        std::vector<std::string> logs;
        if (!get_logs(batch, &logs))
        {
            break;
        }
        else
        {
            MYLOG_DEBUG("%.*s\n", (int)logs[0].size(), logs[0].c_str());

            while (!_stop) // 重试直到成功
            {
                if (lpush_logs(logs))
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    }
    {
        // 线程退出时记录日志
        std::stringstream ss;
        ss << std::this_thread::get_id();
        if (_index == 0)
            MYLOG_INFO("Redis2redisLoader[%d:%s] is exited now\n", _index, ss.str().c_str());
        else
            MYLOG_DEBUG("Redis2redisLoader[%d:%s] is exited now\n", _index, ss.str().c_str());
        kill(getpid(), SIGTERM);
    }
}

bool CFile2redisLoader::get_logs(int batch, std::vector<std::string>* logs)
{
    const char delimiter = mooon::argument::delimiter->value()[0];

    if (_file_offset >= _file_ptr->len)
    {
        return false;
    }
    for (int i=0; i<mooon::argument::batch->value(); ++i)
    {
        const char* start_addr = (const char*)_file_ptr->addr + _file_offset;
        const char* next = strchr(start_addr, delimiter);
        size_t len = 0;
        if (next == NULL)
            len = strlen(start_addr);
        else
            len = next - start_addr;
        if (len > 0)
        {
            const std::string log(start_addr, (std::string::size_type)(len));
            logs->push_back(log);
        }
        if (next == NULL)
            break;
        start_addr += len + 1; // 加 1 跳过换行符
        _file_offset += len + 1;
    }
    return true;
}

bool CFile2redisLoader::lpush_logs(const std::vector<std::string>& logs)
{
    const std::string target_redis_key = get_target_redis_key();
    r3c::Node node;

    try
    {
        // 左进右出
        _redis->lpush(target_redis_key, logs, &node);
        metric.push_number += logs.size();
        return true;
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Redis lpush by %s failed: %s.\n", r3c::node2string(node).c_str(), ex.str().c_str());
        ++metric.redis_exception_number;
        return false;
    }
}

std::string CFile2redisLoader::get_target_redis_key()
{
    std::uniform_int_distribution<int> u(0, mooon::argument::redis_key_count->value()-1);
    const int n = u(_random_engine) % mooon::argument::redis_key_count->value();
    return mooon::utils::CStringUtils::format_string("%s%d", mooon::argument::redis_key_prefix->c_value(), n);
}
