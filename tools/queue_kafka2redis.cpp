// Writed by yijian on 2021/4/25
// Consume kafka to redis list
#include <mooon/net/kafka_consumer.h>
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

// Kafka (source)
STRING_ARG_DEFINE(kafka_brokers, "", "Brokers list of kafka, e.g., --kafka_brokers=127.0.0.1:9092,127.0.0.2:9092/kafka.");
STRING_ARG_DEFINE(kafka_topic, "", "Topic of kafka.");
STRING_ARG_DEFINE(kafka_group, "", "Consumer group of kakfa.");
INTEGER_ARG_DEFINE(int, kafka_timeout, 60000, 0, 86400000, "Timeout to consume kafka in millisecond.");
INTEGER_ARG_DEFINE(int, kafka_offset, 1, 1, 3, "Kafka offset: 1 for earliest, 2 for latest, 3 for none."); // 设置从Kafka哪儿开始消费

// Redis (target)
STRING_ARG_DEFINE(redis_nodes, "", "Nodes list of redi.");
STRING_ARG_DEFINE(redis_password, "", "Password of redis.");
STRING_ARG_DEFINE(redis_key_prefix, "", "Key prefix of redis list.");
INTEGER_ARG_DEFINE(int, redis_key_count, 1, 1, 2020, "Number of redis list keys with the given prefix.");
INTEGER_ARG_DEFINE(int, redis_timeout, 60000, 0, 86400000, "Timeout to lpush redis in millisecond.");

// 批量数，即一次批量移动多少
INTEGER_ARG_DEFINE(int, batch, 1, 1, 100000, "Batch to consume from kafka, and lpush to redis.");

class CKafka2redisConsumer;

struct Metric
{
    std::atomic<uint32_t> consume_number;
    std::atomic<uint32_t> push_number;
    std::atomic<uint32_t> redis_exception_number;
    std::atomic<uint32_t> kafka_error_number;
} metric;

class CKafka2redis: public mooon::sys::CMainHelper
{
private:
    virtual bool on_check_parameter();
    virtual bool on_init(int argc, char* argv[]);
    virtual bool on_run();
    virtual void on_fini();
    virtual void on_terminated();

private:
    bool init_metric_logger();
    bool start_kafka2redis_consumers();
    void stop_kafka2redis_consumers();

private:
    std::shared_ptr<mooon::sys::CSafeLogger> _metric_logger;
    std::vector<std::shared_ptr<CKafka2redisConsumer>> _kafka2redis_consumers;
};

class CKafka2redisConsumer
{
public:
    CKafka2redisConsumer(CKafka2redis* kafka2redis);
    bool start(int index);
    void stop();
    void wait();

private:
    bool init_redis();
    bool init_kafka_consumer();
    void run();

private:
    bool lpush_logs(const std::vector<std::string>& logs);

private:
    int _index;
    CKafka2redis* _kafka2redis;
    std::atomic<bool> _stop;
    std::string _redis_key;
    std::shared_ptr<std::thread> _consume_thread;
    std::shared_ptr<r3c::CRedisClient> _redis;
    std::shared_ptr<mooon::net::CKafkaConsumer> _kafka_consumer;
};

extern "C"
int main(int argc, char* argv[])
{
    CKafka2redis k2r;
    return mooon::sys::main_template(&k2r, argc, argv, NULL);
}

//
// CKafka2redis
//

bool CKafka2redis::on_check_parameter()
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
    // --kafka_group
    if (mooon::argument::kafka_group->value().empty())
    {
        fprintf(stderr, "Parameter[--kafka_group] is not set\n");
        return false;
    }
    if (!mooon::argument::label->value().empty())
    {
        this->set_logger(mooon::argument::label->value());
    }
    return true;
}

bool CKafka2redis::on_init(int argc, char* argv[])
{
    if (!init_metric_logger())
    {
        return false;
    }
    else
    {
        return start_kafka2redis_consumers();
    }
}

bool CKafka2redis::on_run()
{
    while (!this->to_stop())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (metric.consume_number > 0)
        {
            const uint32_t consume_number = metric.consume_number.exchange(0);
            const uint32_t push_number = metric.push_number.exchange(0);
            const uint32_t redis_exception_number = metric.redis_exception_number.exchange(0);
            const uint32_t kafka_error_number = metric.kafka_error_number.exchange(0);
            _metric_logger->log_raw("consume:%u,push:%u,redis:%u,kafka:%u\n", consume_number, push_number, redis_exception_number, kafka_error_number);
        }
    }
    return true;
}

void CKafka2redis::on_fini()
{
    stop_kafka2redis_consumers();
}

void CKafka2redis::on_terminated()
{
    mooon::sys::CMainHelper::on_terminated();
    stop_kafka2redis_consumers();
}

bool CKafka2redis::init_metric_logger()
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

bool CKafka2redis::start_kafka2redis_consumers()
{
    for (int i=0; i<mooon::argument::redis_key_count->value(); ++i)
    {
        std::shared_ptr<CKafka2redisConsumer> kafka2redis_consumer(new CKafka2redisConsumer(this));
        if (kafka2redis_consumer->start(i))
        {
            _kafka2redis_consumers.push_back(kafka2redis_consumer);
        }
        else
        {
            stop_kafka2redis_consumers();
            break;
        }
    }
    return true;
}

void CKafka2redis::stop_kafka2redis_consumers()
{
    for (auto redis2kafka_mover: _kafka2redis_consumers)
    {
        if (redis2kafka_mover.get() != nullptr)
        {
            redis2kafka_mover->stop();
            redis2kafka_mover->wait();
        }
    }
    _kafka2redis_consumers.clear();
}

//
// CKafka2redisConsumer
//

CKafka2redisConsumer::CKafka2redisConsumer(CKafka2redis* kafka2redis)
    : _index(-1), _kafka2redis(kafka2redis), _stop(false)
{
}

bool CKafka2redisConsumer::start(int index)
{
    try
    {
        _index = index;
        _redis_key = mooon::utils::CStringUtils::format_string("%s%d", mooon::argument::redis_key_prefix->c_value(), index);

        if (!init_redis() || !init_kafka_consumer())
        {
            return false;
        }
        else
        {
            _consume_thread.reset(new std::thread(&CKafka2redisConsumer::run, this));
            return true;
        }
    }
    catch (std::system_error& ex)
    {
        MYLOG_ERROR("Start consumer thread(%d) failed: %s\n", index, ex.what());
        return false;
    }
}

void CKafka2redisConsumer::stop()
{
    _stop = true;
}

void CKafka2redisConsumer::wait()
{
    if (_consume_thread.get() != nullptr && _consume_thread->joinable())
    {
        _consume_thread->join();
        _consume_thread.reset();
    }
}

bool CKafka2redisConsumer::init_redis()
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

bool CKafka2redisConsumer::init_kafka_consumer()
{
    std::string str;
    if (mooon::argument::kafka_offset->value() == 1)
        str = "earliest";
    else if (mooon::argument::kafka_offset->value() == 2)
        str = "latest";
    else
        str = "none";
    _kafka_consumer.reset(new mooon::net::CKafkaConsumer);
    _kafka_consumer->set_auto_offset_reset(str);
    return _kafka_consumer->init(
            mooon::argument::kafka_brokers->value(),
            mooon::argument::kafka_topic->value(),
            mooon::argument::kafka_group->value())
        && _kafka_consumer->subscribe_topic();
}

void CKafka2redisConsumer::run()
{
    const int batch = mooon::argument::batch->value();

    for (uint32_t i=0;!_stop;++i)
    {
        const int n = i % mooon::argument::redis_key_count->value();
        const std::string redis_key = mooon::utils::CStringUtils::format_string("%s:%d", mooon::argument::redis_key_prefix->c_value(), n);
        std::vector<std::string> logs;
        r3c::Node node;

        try
        {
            bool timeout;

            if (batch > 1)
            {
                _kafka_consumer->consume_batch(batch, &logs, mooon::argument::kafka_timeout->value(), NULL, &timeout);
            }
            else
            {
                logs.resize(1);
                if (!_kafka_consumer->consume(&logs[0], mooon::argument::kafka_timeout->value(), NULL, &timeout))
                {
                    logs.clear();
                    ++metric.kafka_error_number;
                }
            }
            if (!logs.empty())
            {
                MYLOG_DEBUG("%.*s\n", (int)logs[0].size(), logs[0].c_str());
                metric.consume_number += logs.size();

                while (!_stop) // 重试直到成功
                {
                    if (lpush_logs(logs))
                        break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            }
            else
            {
                if (!timeout)
                    ++metric.kafka_error_number;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        catch (r3c::CRedisException& ex)
        {
            MYLOG_ERROR("Redis lpush by %s failed: %s.\n", r3c::node2string(node).c_str(), ex.str().c_str());
            ++metric.redis_exception_number;
        }
    }
    {
        // 线程退出时记录日志
        std::stringstream ss;
        ss << std::this_thread::get_id();
        if (_index == 0)
            MYLOG_INFO("Kafka2redisConsumer[%d:%s] is exited now\n", _index, ss.str().c_str());
        else
            MYLOG_DEBUG("Kafka2redisConsumer[%d:%s] is exited now\n", _index, ss.str().c_str());
    }
}

bool CKafka2redisConsumer::lpush_logs(const std::vector<std::string>& logs)
{
    r3c::Node node;

    try
    {
        // 左进右出
        _redis->lpush(_redis_key, logs, &node);
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
