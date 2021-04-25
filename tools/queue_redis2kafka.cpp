// Writed by yijian on 2021/4/25
// Move redis list to kafka
#include <mooon/net/kafka_producer.h>
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
#include <sstream>
#include <system_error>
#include <thread>
#include <vector>

// 可选的用来标识进程的字符串
STRING_ARG_DEFINE(label, "", "Optional string used to identify process.");

// Redis (source)
STRING_ARG_DEFINE(redis_nodes, "", "Nodes list of redis");
STRING_ARG_DEFINE(redis_password, "", "Password of redis.");
STRING_ARG_DEFINE(redis_key_prefix, "", "Key prefix of redis list.");
INTEGER_ARG_DEFINE(int, redis_key_count, 1, 1, 2020, "Number of redis list keys with the given prefix.");
INTEGER_ARG_DEFINE(int, redis_timeout, 60000, 0, 86400000, "Timeout to rpop redis in millisecond.");

// Kafka (target)
STRING_ARG_DEFINE(kafka_brokers, "", "Brokers list of kafka, e.g., --kafka_brokers=127.0.0.1:9092,127.0.0.2:9092/kafka.");
STRING_ARG_DEFINE(kafka_topic, "", "Topic of kafka.");
INTEGER_ARG_DEFINE(int, kafka_timeout, 60000, 0, 86400000, "Timeout to produce kafka in millisecond.");

// 批量数，即一次批量移动多少
INTEGER_ARG_DEFINE(int, batch, 1, 1, 100000, "Batch to move from redis to kafka.");

class CRedis2kafkaMover;

struct Metric
{
    std::atomic<uint32_t> pop_number;
    std::atomic<uint32_t> produce_number;
    std::atomic<uint32_t> redis_exception_number;
    std::atomic<uint32_t> kafka_error_number;
} metric;

class CRedis2kafka: public mooon::sys::CMainHelper
{
private:
    virtual bool on_check_parameter();
    virtual bool on_init(int argc, char* argv[]);
    virtual bool on_run();
    virtual void on_fini();
    virtual void on_terminated();

private:
    bool init_metric_logger();
    bool start_redis2kafka_movers();
    void stop_redis2kafka_movers();

private:
    std::shared_ptr<mooon::sys::CSafeLogger> _metric_logger;
    std::vector<std::shared_ptr<CRedis2kafkaMover>> _redis2kafka_movers;
};

class CRedis2kafkaMover
{
public:
    CRedis2kafkaMover(CRedis2kafka* redis2kafka);
    bool start(int index);
    void stop();
    void wait();

private:
    bool init_redis();
    bool init_kafka_producer();
    void run();
    void kafka_produce(const std::vector<std::string>& logs);

private:
    int _index;
    CRedis2kafka* _redis2kafka;
    std::atomic<bool> _stop;
    std::string _redis_key;
    std::shared_ptr<std::thread> _move_thread;
    std::shared_ptr<r3c::CRedisClient> _redis;
    std::shared_ptr<mooon::net::CKafkaProducer> _kafka_producer;
};

extern "C"
int main(int argc, char* argv[])
{
    CRedis2kafka r2k;
    return mooon::sys::main_template(&r2k, argc, argv, NULL);
}

//
// CRedis2kafka
//

bool CRedis2kafka::on_check_parameter()
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
    // --kafka_brokers
    if (mooon::argument::kafka_brokers->value().empty())
    {
        fprintf(stderr, "Parameter[--kafka_brokers] is not set\n");
        return false;
    }
    // --kafka_topic
    if (mooon::argument::kafka_topic->value().empty())
    {
        fprintf(stderr, "Parameter[--kafka_topic] is not set\n");
        return false;
    }
    if (!mooon::argument::label->value().empty())
    {
        this->set_logger(mooon::argument::label->value());
    }
    return true;
}

bool CRedis2kafka::on_init(int argc, char* argv[])
{
    if (!init_metric_logger())
    {
        return false;
    }
    else
    {
        return start_redis2kafka_movers();
    }
}

bool CRedis2kafka::on_run()
{
    while (!this->to_stop())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (metric.pop_number > 0)
        {
            const uint32_t pop_number = metric.pop_number.exchange(0);
            const uint32_t produce_number = metric.produce_number.exchange(0);
            const uint32_t redis_exception_number = metric.redis_exception_number.exchange(0);
            const uint32_t kafka_error_number = metric.kafka_error_number.exchange(0);
            _metric_logger->log_raw("pop:%u,produce:%u,redis:%u,kafka:%u\n", pop_number, produce_number, redis_exception_number, kafka_error_number);
        }
    }
    return true;
}

void CRedis2kafka::on_fini()
{
    stop_redis2kafka_movers();
}

void CRedis2kafka::on_terminated()
{
    mooon::sys::CMainHelper::on_terminated();
    stop_redis2kafka_movers();
}

bool CRedis2kafka::init_metric_logger()
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

bool CRedis2kafka::start_redis2kafka_movers()
{
    for (int i=0; i<mooon::argument::redis_key_count->value(); ++i)
    {
        std::shared_ptr<CRedis2kafkaMover> redis2kafka_mover(new CRedis2kafkaMover(this));
        if (redis2kafka_mover->start(i))
        {
            _redis2kafka_movers.push_back(redis2kafka_mover);
        }
        else
        {
            stop_redis2kafka_movers();
            break;
        }
    }
    return true;
}

void CRedis2kafka::stop_redis2kafka_movers()
{
    for (auto redis2kafka_mover: _redis2kafka_movers)
    {
        if (redis2kafka_mover.get() != nullptr)
        {
            redis2kafka_mover->stop();
            redis2kafka_mover->wait();
        }
    }
    _redis2kafka_movers.clear();
}

//
// CRedis2kafkaMover
//

CRedis2kafkaMover::CRedis2kafkaMover(CRedis2kafka* redis2kafka)
    : _index(-1), _redis2kafka(redis2kafka), _stop(false)
{
}

bool CRedis2kafkaMover::start(int index)
{
    try
    {
        _index = index;
        _redis_key = mooon::utils::CStringUtils::format_string("%s%d", mooon::argument::redis_key_prefix->c_value(), index);

        if (!init_redis() || !init_kafka_producer())
        {
            return false;
        }
        else
        {
            _move_thread.reset(new std::thread(&CRedis2kafkaMover::run, this));
            return true;
        }
    }
    catch (std::system_error& ex)
    {
        MYLOG_ERROR("Start move thread(%d) failed: %s\n", index, ex.what());
        return false;
    }
}

void CRedis2kafkaMover::stop()
{
    _stop = true;
}

void CRedis2kafkaMover::wait()
{
    if (_move_thread.get() != nullptr && _move_thread->joinable())
    {
        _move_thread->join();
        _move_thread.reset();
    }
}

bool CRedis2kafkaMover::init_redis()
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

bool CRedis2kafkaMover::init_kafka_producer()
{
    std::string errmsg;
    _kafka_producer.reset(new mooon::net::CKafkaProducer);
    if (_kafka_producer->init(mooon::argument::kafka_brokers->value(), mooon::argument::kafka_topic->value(), &errmsg))
    {
        return true;
    }
    else
    {
        MYLOG_ERROR("Init KafkaProducer failed: %s\n", errmsg.c_str());
        return false;
    }
}

void CRedis2kafkaMover::run()
{
    const int batch = mooon::argument::batch->value();

    while (!_stop)
    {
        std::vector<std::string> logs(batch);
        r3c::Node node;

        try
        {
            // 左进右出
            if (batch == 1)
                _redis->rpop(_redis_key, &logs[0], &node);
            else
                _redis->rpop(_redis_key, &logs, batch, &node);
            if (!logs.empty())
                kafka_produce(logs);
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        catch (r3c::CRedisException& ex)
        {
            MYLOG_ERROR("Redis rpop by %s failed: %s.\n", r3c::node2string(node).c_str(), ex.str().c_str());
            ++metric.redis_exception_number;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    {
        // 线程退出时记录日志
        std::stringstream ss;
        ss << std::this_thread::get_id();
        if (_index == 0)
            MYLOG_INFO("Redis2kafkaMover[%d:%s] is exited now\n", _index, ss.str().c_str());
        else
            MYLOG_DEBUG("Redis2kafkaMover[%d:%s] is exited now\n", _index, ss.str().c_str());
    }
}

void CRedis2kafkaMover::kafka_produce(const std::vector<std::string>& logs)
{
    metric.pop_number += logs.size();

    while (true)
    {
        std::string errmsg;
        int errcode = 0;

        if (logs.size() == 1)
            _kafka_producer->produce("", logs[0], RdKafka::Topic::PARTITION_UA, &errcode, &errmsg);
        else if (logs.size() > 1)
            _kafka_producer->produce_batch("", logs, RdKafka::Topic::PARTITION_UA, &errcode, &errmsg);
        if (errcode == 0)
        {
            metric.produce_number += logs.size();
            break;
        }
        else
        {
            MYLOG_ERROR("Kafka error: (%d)%s\n", errcode, errmsg.c_str());
            ++metric.kafka_error_number;
        }
        if (errcode != RdKafka::ERR__QUEUE_FULL)
        {
            break;
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
