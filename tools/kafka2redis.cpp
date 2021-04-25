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

// Kafka (source)
STRING_ARG_DEFINE(kafkabrokers, "", "Brokers list of kafka, e.g., --kafkabrokers=127.0.0.1:9092,127.0.0.2:9092/kafka.");
STRING_ARG_DEFINE(kafkatopic, "", "Topic of kafka.");
STRING_ARG_DEFINE(kafkagroup, "", "Consumer group of kakfa.");
INTEGER_ARG_DEFINE(int, kafkatimeout, 60000, 0, 86400000, "Timeout to consume kafka in millisecond.");
INTEGER_ARG_DEFINE(int, kafkaoffset, 1, 1, 3, "Kafka offset: 1 for earliest, 2 for latest, 3 for none."); // 设置从Kafka哪儿开始消费

// Redis (destination)
STRING_ARG_DEFINE(redis, "", "Nodes list of redi.");
STRING_ARG_DEFINE(redispassword, "", "Password of redis.");
STRING_ARG_DEFINE(prefix, "", "Key prefix of redis list.");
INTEGER_ARG_DEFINE(int, keys, 1, 1, 2020, "Number of redis list keys with the given prefix.");
INTEGER_ARG_DEFINE(int, redistimeout, 60000, 0, 86400000, "Timeout to consume redis in millisecond.");

// 批量数，即一次批量移动多少
INTEGER_ARG_DEFINE(int, batch, 1, 1, 100000, "Batch to move from redis to kafka.");

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
    // --redis
    if (mooon::argument::redis->value().empty())
    {
        fprintf(stderr, "Parameter[--redis] is not set\n");
        return false;
    }
    // --prefix
    if (mooon::argument::prefix->value().empty())
    {
        fprintf(stderr, "Parameter[--prefix] is not set\n");
        return false;
    }

    // --kafkabrokers
    if (mooon::argument::kafkabrokers->value().empty())
    {
        fprintf(stderr, "Parameter[--kafkabrokers] is not set\n");
        return false;
    }
    // --kafkatopic
    if (mooon::argument::kafkatopic->value().empty())
    {
        fprintf(stderr, "Parameter[--kafkatopic] is not set\n");
        return false;
    }
    // --kafkagroup
    if (mooon::argument::kafkagroup->value().empty())
    {
        fprintf(stderr, "Parameter[--kafkagroup] is not set\n");
        return false;
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

bool CKafka2redis::start_kafka2redis_consumers()
{
    for (int i=0; i<mooon::argument::keys->value(); ++i)
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
        _redis_key = mooon::utils::CStringUtils::format_string("%s%d", mooon::argument::prefix->c_value(), index);

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
                        mooon::argument::redis->value(),
                        mooon::argument::redispassword->value(),
                        mooon::argument::redistimeout->value(),
                        mooon::argument::redistimeout->value()));
        return true;
    }
    catch (r3c::CRedisException& ex)
    {
        MYLOG_ERROR("Create RedisClient by %s failed: %s.\n", mooon::argument::redis->c_value(), ex.str().c_str());
        return false;
    }
}

bool CKafka2redisConsumer::init_kafka_consumer()
{
    std::string str;
    if (mooon::argument::kafkaoffset->value() == 1)
        str = "earliest";
    else if (mooon::argument::kafkaoffset->value() == 2)
        str = "latest";
    else
        str = "none";
    _kafka_consumer.reset(new mooon::net::CKafkaConsumer);
    _kafka_consumer->set_auto_offset_reset(str);
    return _kafka_consumer->init(
            mooon::argument::kafkabrokers->value(),
            mooon::argument::kafkatopic->value(),
            mooon::argument::kafkagroup->value())
        && _kafka_consumer->subscribe_topic();
}

void CKafka2redisConsumer::run()
{
    const int batch = mooon::argument::batch->value();

    for (uint32_t i=0;!_stop;++i)
    {
        const int n = i % mooon::argument::keys->value();
        const std::string redis_key = mooon::utils::CStringUtils::format_string("%s:%d", mooon::argument::prefix->c_value(), n);
        std::vector<std::string> logs;
        r3c::Node node;

        try
        {
            bool timeout;

            if (batch > 1)
            {
                _kafka_consumer->consume_batch(batch, &logs, mooon::argument::kafkatimeout->value(), NULL, &timeout);
            }
            else
            {
                logs.resize(1);
                if (!_kafka_consumer->consume(&logs[0], mooon::argument::kafkatimeout->value(), NULL, &timeout))
                {
                    logs.clear();
                    ++metric.kafka_error_number;
                }
            }
            if (!logs.empty())
            {
                metric.consume_number += logs.size();
                _redis->lpush(redis_key, logs, &node);
                metric.push_number += logs.size();
            }
            else if (!timeout)
            {
                ++metric.kafka_error_number;
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
