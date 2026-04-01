// Writed by yijian on 2021/5/20
// Move redis list to file
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
STRING_ARG_DEFINE(file, "", "File path to store.");

// Redis (target)
STRING_ARG_DEFINE(redis_nodes, "", "Nodes list of redis.");
STRING_ARG_DEFINE(redis_password, "", "Password of redis.");
STRING_ARG_DEFINE(redis_key_prefix, "", "Key prefix of redis list.");
INTEGER_ARG_DEFINE(int, redis_key_count, 1, 1, 2020, "Number of redis list keys with the given prefix.");
INTEGER_ARG_DEFINE(int, redis_timeout, 60000, 0, 86400000, "Timeout to lpush redis in millisecond.");
// 批量数，即一次批量移动多少
INTEGER_ARG_DEFINE(int, batch, 1, 1, 100000, "Batch to move from redis to file.");

// metric 统计间隔（单位：秒）
INTEGER_ARG_DEFINE(int, interval, 10, 0, 3600, "Interval to count metric in seconds.");

// 分割符合
STRING_ARG_DEFINE(delimiter, "\n", "Single character separator of logs.");

class CRedis2fileMover;

struct Metric
{
    std::atomic<uint32_t> pop_number;
    std::atomic<uint32_t> redis_exception_number;
} metric;

class CRedis2file: public mooon::sys::CMainHelper
{
private:
    virtual bool on_check_parameter();
    virtual bool on_init(int argc, char* argv[]);
    virtual bool on_run();
    virtual void on_fini();
    virtual void on_terminated();

private:
    bool init_metric_logger();
    bool start_redis2file_movers();
    void stop_redis2file_movers();
    void wait_redis2file_movers();

private:
    std::shared_ptr<mooon::sys::CSafeLogger> _metric_logger;
    std::vector<std::shared_ptr<CRedis2fileMover>> _redis2file_movers;
};

class CRedis2fileMover
{
public:
    CRedis2fileMover(CRedis2file* redis2redis);
    ~CRedis2fileMover();
    bool start(time_t now, int index);
    void stop();
    void wait();

private:
    bool init_source_redis();
    bool init_target_file();
    void run();

private:
    bool rpop_logs(int batch, std::vector<std::string>* logs);
    bool write_logs(const std::vector<std::string>& logs);

private:
    int _index;
    CRedis2file* _redis2file;
    std::atomic<bool> _stop;
    std::string _redis_key;
    std::shared_ptr<std::thread> _move_thread;
    std::shared_ptr<r3c::CRedisClient> _redis;
    std::default_random_engine _random_engine;
    int _target_fd;
};

extern "C"
int main(int argc, char* argv[])
{
    CRedis2file r2f;
    return mooon::sys::main_template(&r2f, argc, argv, NULL);
}

//
// CRedis2redis
//

bool CRedis2file::on_check_parameter()
{
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

bool CRedis2file::on_init(int argc, char* argv[])
{
    if (!init_metric_logger())
    {
        return false;
    }
    else
    {
        return start_redis2file_movers();
    }
}

bool CRedis2file::on_run()
{
    const int interval = mooon::argument::interval->value();

    MYLOG_INFO("Redis2file is started now\n");
    if (interval <= 0)
    {
        wait_redis2file_movers();
    }
    else
    {
        wait_redis2file_movers();
        kill(getpid(), SIGTERM);
        while (!this->to_stop())
        {
            for (int i=0; i<interval&&!this->to_stop(); ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            if (metric.pop_number > 0)
            {
                const uint32_t pop_number = metric.pop_number.exchange(0);
                const uint32_t redis_exception_number = metric.redis_exception_number.exchange(0);
                _metric_logger->log_raw("pop:%u,redis:%u\n", pop_number, redis_exception_number);
            }
        }
    }
    return true;
}

void CRedis2file::on_fini()
{
    stop_redis2file_movers();
    wait_redis2file_movers();
}

void CRedis2file::on_terminated()
{
    mooon::sys::CMainHelper::on_terminated();
    stop_redis2file_movers();
}

bool CRedis2file::init_metric_logger()
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

bool CRedis2file::start_redis2file_movers()
{
    const time_t now = time(NULL);
    for (int i=0; i<mooon::argument::redis_key_count->value(); ++i)
    {
        std::shared_ptr<CRedis2fileMover> redis2redis_mover(new CRedis2fileMover(this));
        if (redis2redis_mover->start(now, i))
        {
            _redis2file_movers.push_back(redis2redis_mover);
        }
        else
        {
            stop_redis2file_movers();
            break;
        }
    }
    return true;
}

void CRedis2file::stop_redis2file_movers()
{
    for (auto redis2redis_mover: _redis2file_movers)
    {
        if (redis2redis_mover.get() != nullptr)
            redis2redis_mover->stop();
    }
}

void CRedis2file::wait_redis2file_movers()
{
    for (auto redis2redis_mover: _redis2file_movers)
    {
        if (redis2redis_mover.get() != nullptr)
            redis2redis_mover->wait();
    }
    _redis2file_movers.clear();
}

//
// CRedis2redisMover
//

CRedis2fileMover::CRedis2fileMover(CRedis2file* redis2redis)
    : _index(-1), _redis2file(redis2redis), _stop(false),
      _target_fd(-1)
{
}

CRedis2fileMover::~CRedis2fileMover()
{
    if (_target_fd != -1)
        close(_target_fd);
}

bool CRedis2fileMover::start(time_t now, int index)
{
    try
    {
        _index = index;
        _redis_key = mooon::utils::CStringUtils::format_string("%s%d", mooon::argument::redis_key_prefix->c_value(), index);

        if (!init_source_redis() || !init_target_file())
        {
            return false;
        }
        else
        {
            _move_thread.reset(new std::thread(&CRedis2fileMover::run, this));
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

void CRedis2fileMover::stop()
{
    _stop = true;
}

void CRedis2fileMover::wait()
{
    if (_move_thread.get() != nullptr && _move_thread->joinable())
    {
        _move_thread->join();
        _move_thread.reset();
    }
}

bool CRedis2fileMover::init_source_redis()
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

bool CRedis2fileMover::init_target_file()
{
    _target_fd = open(mooon::argument::file->c_value(), FILE_DEFAULT_PERM | O_WRONLY | O_APPEND | O_CREAT);
    if (_target_fd != -1)
    {
        return true;
    }
    else
    {
        if (errno == EEXIST)
        {
            _target_fd = open(mooon::argument::file->c_value(), FILE_DEFAULT_PERM | O_WRONLY | O_APPEND);
        }
        if (_target_fd != -1)
        {
            return true;
        }
        else
        {
            MYLOG_ERROR("Open file://%s failed: %s\n", mooon::argument::file->c_value(), strerror(errno));
            return false;
        }
    }
}

void CRedis2fileMover::run()
{
    const int batch = mooon::argument::batch->value();

    while (!_stop)
    {
        std::vector<std::string> logs(batch);
        if (!rpop_logs(batch, &logs))
        {
            break;
        }
        else
        {
            MYLOG_DEBUG("%.*s\n", (int)logs[0].size(), logs[0].c_str());

            if (!write_logs(logs))
                break;
        }
    }
    {
        // 线程退出时记录日志
        std::stringstream ss;
        ss << std::this_thread::get_id();
        if (_index == 0)
            MYLOG_INFO("Redis2fileMover[%d:%s] is exited now\n", _index, ss.str().c_str());
        else
            MYLOG_DEBUG("Redis2fileMover[%d:%s] is exited now\n", _index, ss.str().c_str());
    }
}

bool CRedis2fileMover::rpop_logs(int batch, std::vector<std::string>* logs)
{
    r3c::Node node;

    try
    {
        int n = 0;

        // 左进右出
        if (batch > 1)
        {
            n = _redis->rpop(_redis_key, logs, batch, &node);
        }
        else
        {
            if (_redis->rpop(_redis_key, &(*logs)[0], &node))
                n = 1;
        }
        metric.pop_number += n;
        return n > 0;
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Redis rpop by %s failed: %s.\n", r3c::node2string(node).c_str(), ex.str().c_str());
        ++metric.redis_exception_number;
        return true;
    }
}

bool CRedis2fileMover::write_logs(const std::vector<std::string>& logs)
{
    for (auto log: logs)
    {
        log.append(mooon::argument::delimiter->c_value());
        if (write(_target_fd, log.c_str(), log.size()) == -1)
        {
            MYLOG_ERROR("Write file://%s failed: %s\n", mooon::argument::file->c_value() , strerror(errno));
            return false;
        }
    }

    return true;
}
