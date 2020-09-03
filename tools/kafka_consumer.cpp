// Writed by yijian on 2020/9/2
// Kafka消费者，可用于测试Kafka的消费性能
#include <mooon/net/kafka_consumer.h>
#include <mooon/sys/atomic.h>
#include <mooon/sys/safe_logger.h>
#include <mooon/sys/stop_watch.h>
#include <mooon/sys/thread_engine.h>
#include <mooon/utils/args_parser.h>

// 指定 Kafka 的 Brokers列表
STRING_ARG_DEFINE(kafka, "", "Kafka brokers, example: --kafka=\"192.168.1.10:9092,192.168.1.11:9092\".");

// 指定目标 Topic
STRING_ARG_DEFINE(topic, "", "Kafka topic.");

// 消费者组
STRING_ARG_DEFINE(group, "", "Consumer group.");

// 消息方式
INTEGER_ARG_DEFINE(int, offset, 0, 0, 2, "Auto offset: 0 none 1 earliest 2 latest, example: --offset=1.");

// 消息数（总的要消费的消息数）
INTEGER_ARG_DEFINE(int, num, 1, 1, 100000000, "Number of messages consumed.");

// 每次消费多少笔
INTEGER_ARG_DEFINE(int, batch, 1, 1, 2020, "Batch to consume.");

// Kafka 消费超时时长
INTEGER_ARG_DEFINE(int, timeout, 1000, 1, 3600000, "Timeout in millisecond to consume messages.");

#if MOOON_HAVE_LIBRDKAFKA==1 // 宏MOOON_HAVE_LIBRDKAFKA的值须为1

static std::string get_offset_str();
static int get_num_threads(int num_partitions);
static int get_num_partitions();
static bool start_consumer_threads();
static void wait_consumer_threads();
static void consumer_thread_proc(int index, int num);
static void summarize(mooon::sys::CStopWatch* sw);

static std::vector<mooon::sys::CThreadEngine*> consumer_threads;
static mooon::sys::CAtomic<int> num_logs_consumed;

extern "C"
int main(int argc, char* argv[])
{
    std::string errmsg;

    if (!mooon::utils::parse_arguments(argc, argv, &errmsg))
    {
        if (!errmsg.empty())
            fprintf(stderr, "%s.\n", errmsg.c_str());
        fprintf(stderr, "%s.\n", mooon::utils::g_help_string.c_str());
        return 1;
    }
    // --kafka
    if (mooon::argument::kafka->value().empty())
    {
        fprintf(stderr, "Parameter[--kafka] is not set.\n");
        fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        return 1;
    }
    // --topic
    if (mooon::argument::topic->value().empty())
    {
        fprintf(stderr, "Parameter[--topic] is not set.\n");
        fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        return 1;
    }
    // --group
    if (mooon::argument::group->value().empty())
    {
        fprintf(stderr, "Parameter[--group] is not set.\n");
        fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        return 1;
    }
    else
    {
        mooon::sys::g_logger = mooon::sys::create_safe_logger();
        mooon::sys::CStopWatch sw(&summarize);

        if (!start_consumer_threads())
        {
            return 1;
        }
        else
        {
            wait_consumer_threads();
            return 0;
        }
    }
}

std::string get_offset_str()
{
    const int offset = mooon::argument::offset->value();
    if (offset == 1)
        return "none";
    else if (offset == 2)
        return "earliest";
    else if (offset == 2)
        return "latest";
    else
        return "none";
}

int get_num_threads(int num_partitions)
{
    const int num = mooon::argument::num->value();
    if (num < num_partitions)
        return num;
    else
        return num_partitions;
}

int get_num_partitions()
{
    int num_partitions = -1;
    mooon::net::CKafkaConsumer kafka_consumer;

    if (!kafka_consumer.init(mooon::argument::kafka->value(), mooon::argument::topic->value(), mooon::argument::group->value()))
    {
        MYLOG_ERROR("Init consumer://%s with topic://%s failed\n", mooon::argument::kafka->c_value(), mooon::argument::topic->c_value());
        num_partitions = -1;
    }
    else
    {
        std::string errmsg;
        const int n = kafka_consumer.get_num_partitions(&errmsg);
        if (n == -1)
        {
            MYLOG_ERROR("Get number of partitions of topic://%s failed: %s\n", mooon::argument::topic->c_value(), errmsg.c_str());
            num_partitions = -1;
        }
        else
        {
            MYLOG_INFO("Number of partitions of topic://%s: %d\n", mooon::argument::topic->c_value(), n);
            num_partitions = n;
        }

        kafka_consumer.close();
    }

    return num_partitions;
}

bool start_consumer_threads()
{
    try
    {
        const int num_partitions = get_num_partitions();

        if (num_partitions == -1)
        {
            return false;
        }
        else
        {
            const int num_threads = get_num_threads(num_partitions);
            const int total = mooon::argument::num->value();

            for (int i=0; i<num_threads; ++i)
            {
                int num = 0;
                if (i == 0)
                    num = (total / num_threads) + (total % num_threads);
                else
                    num = (total / num_threads);
                mooon::sys::CThreadEngine* consumer_thread = new mooon::sys::CThreadEngine(mooon::sys::bind(&consumer_thread_proc, i, num));
                consumer_threads.push_back(consumer_thread);
            }
            return true;
        }
    }
    catch (mooon::sys::CSyscallException& ex)
    {
        MYLOG_ERROR("Start consumer thread failed: %s\n", ex.str().c_str());
        return false;
    }
}

void wait_consumer_threads()
{
    const int num_threads = static_cast<int>(consumer_threads.size());
    for (int i=0; i<num_threads; ++i)
    {
        consumer_threads[i]->join();
        delete consumer_threads[i];
    }
    consumer_threads.clear();
}

void consumer_thread_proc(int index, int num)
{
    const std::string offset_str = get_offset_str();
    mooon::net::CKafkaConsumer kafka_consumer;

    if (offset_str != "none")
    {
        kafka_consumer.set_auto_offset_reset(get_offset_str());
    }
    if (!kafka_consumer.init(mooon::argument::kafka->value(), mooon::argument::topic->value(), mooon::argument::group->value()))
    {
        MYLOG_ERROR("Init consumer://%s with topic://%s failed\n", mooon::argument::kafka->c_value(), mooon::argument::topic->c_value());
    }
    else
    {
        const int timeout_ms = mooon::argument::timeout->value();
        const int batch = mooon::argument::batch->value();

        for (int i=0; i<num; ++i)
        {
            struct mooon::net::MessageInfo mi;
            std::vector<std::string> logs;
            std::string log;

            if (batch == 1)
            {
                if (kafka_consumer.consume(&log, timeout_ms, &mi))
                    ++num_logs_consumed;
            }
            else
            {
                num_logs_consumed += kafka_consumer.consume_batch(batch, &logs, timeout_ms, &mi);
            }
        }
    }
}

void summarize(mooon::sys::CStopWatch* sw)
{
    const int n = num_logs_consumed.get_value();
    const uint64_t us = sw->get_elapsed_microseconds();
    const uint64_t ms = us / 1000;
    const uint64_t s = ms / 1000;
    const int tps = (s>0)? (n/s): n;
    MYLOG_INFO("Number: %d\n", n);
    MYLOG_INFO("Time: %" PRIu64" us, %" PRIu64"ms, %" PRIu64"s\n", us, ms, s);
    MYLOG_INFO("TPS: %d\n", tps);
}

#endif // MOOON_HAVE_LIBRDKAFKA==1
