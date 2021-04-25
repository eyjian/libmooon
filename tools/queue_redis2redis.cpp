// Writed by yijian on 2021/4/25
// Move redis list from one to another
#include <mooon/sys/main_template.h>
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
STRING_ARG_DEFINE(source_redis_nodes, "", "Nodes list of source redis");
STRING_ARG_DEFINE(source_redis_password, "", "Password of source redis.");
STRING_ARG_DEFINE(source_redis_key_prefix, "", "Key prefix of source redis list.");
INTEGER_ARG_DEFINE(int, source_redis_key_count, 1, 1, 2020, "Number of source redis list keys with the given prefix.");
INTEGER_ARG_DEFINE(int, source_redis_timeout, 60000, 0, 86400000, "Timeout to rpop source redis in millisecond.");

// Redis (target)
STRING_ARG_DEFINE(target_redis_nodes, "", "Nodes list of target redis.");
STRING_ARG_DEFINE(target_redis_password, "", "Password of target redis.");
STRING_ARG_DEFINE(target_redis_key_prefix, "", "Key prefix of target redis list.");
INTEGER_ARG_DEFINE(int, target_redis_key_count, 1, 1, 2020, "Number of target redis list keys with the given prefix.");
INTEGER_ARG_DEFINE(int, target_redis_timeout, 60000, 0, 86400000, "Timeout to lpush target redis in millisecond.");

// 批量数，即一次批量移动多少
INTEGER_ARG_DEFINE(int, batch, 1, 1, 100000, "Batch to move from redis to kafka.");

class CRedis2redisMover;

struct Metric
{
    std::atomic<uint32_t> pop_number;
    std::atomic<uint32_t> push_number;
    std::atomic<uint32_t> source_redis_exception_number;
    std::atomic<uint32_t> target_redis_exception_number;
} metric;

class CRedis2redis: public mooon::sys::CMainHelper
{
private:
    virtual bool on_check_parameter();
    virtual bool on_init(int argc, char* argv[]);
    virtual bool on_run();
    virtual void on_fini();
    virtual void on_terminated();

private:
    bool init_metric_logger();
    bool start_redis2redis_movers();
    void stop_redis2redis_movers();

private:
    std::shared_ptr<mooon::sys::CSafeLogger> _metric_logger;
    std::vector<std::shared_ptr<CRedis2redisMover>> _redis2redis_movers;
};

class CRedis2redisMover
{
public:
    CRedis2redisMover(CRedis2redis* redis2redis);
    bool start(time_t now, int index);
    void stop();
    void wait();

private:
    bool init_source_redis();
    bool init_target_redis();
    void run();

private:
    bool rpop_logs(int batch, std::vector<std::string>* logs);
    void lpush_logs(const std::vector<std::string>& logs);
    std::string get_target_redis_key();

private:
    int _index;
    CRedis2redis* _redis2redis;
    std::atomic<bool> _stop;
    std::string _source_redis_key;
    std::shared_ptr<std::thread> _move_thread;
    std::shared_ptr<r3c::CRedisClient> _source_redis;
    std::shared_ptr<r3c::CRedisClient> _target_redis;
    std::default_random_engine _random_engine;
};

extern "C"
int main(int argc, char* argv[])
{
    CRedis2redis r2r;
    return mooon::sys::main_template(&r2r, argc, argv, NULL);
}

//
// CRedis2redis
//

bool CRedis2redis::on_check_parameter()
{
    // --source_redis_nodes
    if (mooon::argument::source_redis_nodes->value().empty())
    {
        fprintf(stderr, "Parameter[--source_redis_nodes] is not set\n");
        return false;
    }
    // --source_redis_key_prefix
    if (mooon::argument::source_redis_key_prefix->value().empty())
    {
        fprintf(stderr, "Parameter[--source_redis_key_prefix] is not set\n");
        return false;
    }
    // --target_redis_nodes
    if (mooon::argument::target_redis_nodes->value().empty())
    {
        fprintf(stderr, "Parameter[--target_redis_nodes] is not set\n");
        return false;
    }
    // --target_redis_key_prefix
    if (mooon::argument::target_redis_key_prefix->value().empty())
    {
        fprintf(stderr, "Parameter[--target_redis_key_prefix] is not set\n");
        return false;
    }
    if (!mooon::argument::label->value().empty())
    {
        this->set_logger(mooon::argument::label->value());
    }
    return true;
}

bool CRedis2redis::on_init(int argc, char* argv[])
{
    if (!init_metric_logger())
    {
        return false;
    }
    else
    {
        return start_redis2redis_movers();
    }
}

bool CRedis2redis::on_run()
{
    while (!this->to_stop())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (metric.pop_number > 0)
        {
            const uint32_t pop_number = metric.pop_number.exchange(0);
            const uint32_t push_number = metric.push_number.exchange(0);
            const uint32_t source_redis_exception_number = metric.source_redis_exception_number.exchange(0);
            const uint32_t target_redis_exception_number = metric.target_redis_exception_number.exchange(0);
            _metric_logger->log_raw("pop:%u,push:%u,source:%u,target:%u\n", pop_number, push_number, source_redis_exception_number, target_redis_exception_number);
        }
    }
    return true;
}

void CRedis2redis::on_fini()
{
    stop_redis2redis_movers();
}

void CRedis2redis::on_terminated()
{
    mooon::sys::CMainHelper::on_terminated();
    stop_redis2redis_movers();
}

bool CRedis2redis::init_metric_logger()
{
    try
    {
        _metric_logger.reset(mooon::sys::create_safe_logger(true, mooon::SIZE_1K, "metric"));
        _metric_logger->enable_raw_log(true, true);
        return true;
    }
    catch (mooon::sys::CSyscallException& ex)
    {
        MYLOG_ERROR("Create metric logger failed: %s\n", ex.str().c_str());
        return false;
    }
}

bool CRedis2redis::start_redis2redis_movers()
{
    const time_t now = time(NULL);
    for (int i=0; i<mooon::argument::source_redis_key_count->value(); ++i)
    {
        std::shared_ptr<CRedis2redisMover> redis2redis_mover(new CRedis2redisMover(this));
        if (redis2redis_mover->start(now, i))
        {
            _redis2redis_movers.push_back(redis2redis_mover);
        }
        else
        {
            stop_redis2redis_movers();
            break;
        }
    }
    return true;
}

void CRedis2redis::stop_redis2redis_movers()
{
    for (auto redis2redis_mover: _redis2redis_movers)
    {
        if (redis2redis_mover.get() != nullptr)
        {
            redis2redis_mover->stop();
            redis2redis_mover->wait();
        }
    }
    _redis2redis_movers.clear();
}

//
// CRedis2redisMover
//

CRedis2redisMover::CRedis2redisMover(CRedis2redis* redis2redis)
    : _index(-1), _redis2redis(redis2redis), _stop(false)
{
}

bool CRedis2redisMover::start(time_t now, int index)
{
    try
    {
        _index = index;
        _source_redis_key = mooon::utils::CStringUtils::format_string("%s%d", mooon::argument::source_redis_key_prefix->c_value(), index);

        if (!init_source_redis() || !init_target_redis())
        {
            return false;
        }
        else
        {
            _move_thread.reset(new std::thread(&CRedis2redisMover::run, this));
            _random_engine.seed(now+index);
            return true;
        }
    }
    catch (std::system_error& ex)
    {
        MYLOG_ERROR("Start move thread(%d) failed: %s\n", index, ex.what());
        return false;
    }
}

void CRedis2redisMover::stop()
{
    _stop = true;
}

void CRedis2redisMover::wait()
{
    if (_move_thread.get() != nullptr && _move_thread->joinable())
    {
        _move_thread->join();
        _move_thread.reset();
    }
}

bool CRedis2redisMover::init_source_redis()
{
    try
    {
        _source_redis.reset(
                new r3c::CRedisClient(
                        mooon::argument::source_redis_nodes->value(),
                        mooon::argument::source_redis_password->value(),
                        mooon::argument::source_redis_timeout->value(),
                        mooon::argument::source_redis_timeout->value()));
        return true;
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Create source RedisClient by %s failed: %s.\n", mooon::argument::source_redis_nodes->c_value(), ex.str().c_str());
        return false;
    }
}

bool CRedis2redisMover::init_target_redis()
{
    try
    {
        _target_redis.reset(
                new r3c::CRedisClient(
                        mooon::argument::target_redis_nodes->value(),
                        mooon::argument::target_redis_password->value(),
                        mooon::argument::target_redis_timeout->value(),
                        mooon::argument::target_redis_timeout->value()));
        return true;
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Create target RedisClient by %s failed: %s.\n", mooon::argument::target_redis_nodes->c_value(), ex.str().c_str());
        return false;
    }
}

void CRedis2redisMover::run()
{
    const int batch = mooon::argument::batch->value();

    while (!_stop)
    {
        std::vector<std::string> logs(batch);
        if (rpop_logs(batch, &logs))
            lpush_logs(logs);
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    {
        // 线程退出时记录日志
        std::stringstream ss;
        ss << std::this_thread::get_id();
        if (_index == 0)
            MYLOG_INFO("Redis2redisMover[%d:%s] is exited now\n", _index, ss.str().c_str());
        else
            MYLOG_DEBUG("Redis2redisMover[%d:%s] is exited now\n", _index, ss.str().c_str());
    }
}

bool CRedis2redisMover::rpop_logs(int batch, std::vector<std::string>* logs)
{
    r3c::Node node;

    try
    {
        // 左进右出
        if (batch == 1)
            _source_redis->rpop(_source_redis_key, &(*logs)[0], &node);
        else
            _source_redis->rpop(_source_redis_key, logs, batch, &node);
        metric.pop_number += logs->size();
        return !logs->empty();
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Redis rpop by %s failed: %s.\n", r3c::node2string(node).c_str(), ex.str().c_str());
        ++metric.source_redis_exception_number;
        return true;
    }
}

void CRedis2redisMover::lpush_logs(const std::vector<std::string>& logs)
{
    const std::string target_redis_key = get_target_redis_key();
    r3c::Node node;

    try
    {
        // 左进右出
        _target_redis->lpush(target_redis_key, logs, &node);
        metric.push_number += logs.size();
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Redis lpush by %s failed: %s.\n", r3c::node2string(node).c_str(), ex.str().c_str());
        ++metric.target_redis_exception_number;
    }
}

std::string CRedis2redisMover::get_target_redis_key()
{
    std::uniform_int_distribution<int> u(0, mooon::argument::target_redis_key_count->value());
    const int n = u(_random_engine) % mooon::argument::target_redis_key_count->value();
    return mooon::utils::CStringUtils::format_string("%s%d", mooon::argument::target_redis_key_prefix->c_value(), n);
}
