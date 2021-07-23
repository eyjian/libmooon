// Writed by yijian on 2019/8/5
#ifndef MOOON_NET_KAFKA_CONSUMER_H
#define MOOON_NET_KAFKA_CONSUMER_H
#if MOOON_HAVE_LIBRDKAFKA==1 // 宏MOOON_HAVE_LIBRDKAFKA的值须为1
#include <mooon/net/config.h>
#include <mooon/utils/scoped_ptr.h>
#include <librdkafka/rdkafkacpp.h>
NET_NAMESPACE_BEGIN

class DefEventImpl;
class DefConsumeImpl;
class DefRebalanceImpl;
class DefOffsetCommitImpl;

struct MessageInfo
{
    void* message;
    int32_t partition;
    int64_t offset;
    int64_t timestamp;
    std::string topicname;
    std::string key;

    MessageInfo();
    ~MessageInfo();
};

// 注：一个CKafkaConsumer实例只能消费一个topic，
// 被消息的topic由init成员函数指定。
//
// Kafka 有两种消费方式：
// 1）不绑定分区的订阅消费，在init成功后调用subscribe_topic
// 2）绑定分区的消费，在init成功后调用assign_partitions
//
// 一个线程可以消费一或多个分区，但一个分区只能被一个线程消费。
class CKafkaConsumer
{
public:
    // CKafkaProducer 负责event_cb、consume_cb、rebalance_cb 和 offset_commitcb 的销毁。
    //
    // 如果 event_cb 为空，则使用 DefEventImpl 作为 EventCb
    // 如果 consume_cb 为空，则使用 DefConsumeImpl 作为 ConsumeImpl，
    // 如果 rebalance_cb 为空，则使用 DefRebalanceImpl 作为 RebalanceImpl，
    // 如果 offset_commitcb 为空，则使用 DefOffsetCommitImpl 作为 OffsetCommitImpl。
    CKafkaConsumer(RdKafka::EventCb* event_cb=NULL, RdKafka::ConsumeCb* consume_cb=NULL, RdKafka::RebalanceCb* rebalance_cb=NULL, RdKafka::OffsetCommitCb* offset_commitcb=NULL);
    ~CKafkaConsumer();

    // 设置配置 auto.offset.reset 的值，
    // set_auto_offset_reset 需在 init 之前调用。
    //
    // str可取值：
    // 1) earliest 当各分区下有已提交的offset时从提交的offset开始消费，无提交的offset时则从头开始消费
    // 2) latest 默认值，当各分区下有已提交的offset时从提交的offset开始消费，无提交的offset时则消费新产生的该分区下的数据
    // 3) none Topic各分区都存在已提交的offset时从offset后开始消费，只要有一个分区不存在已提交的offset则抛出异常
    void set_auto_offset_reset(const std::string& str);

    // close 调用最长会阻塞 session.timeout.ms 指定的时长（毫秒）
    // 在调用 close 之前应先调用 unsubscribe_topic 或 unassign_partitions。
    void close();
    bool init(const std::string& brokers, const std::string& topic, const std::string& group, bool enable_rebalance=false, bool enable_auto_commit=true);

    // 以订阅方式消费
    bool subscribe_topic(); // init成功后调用
    bool unsubscribe_topic(); // 如果是订阅消费模式，则在close之前应调用

    // init成功后调用，partitions为分区ID数组（从0开始到分区数减一）
    bool assign_partitions(const std::vector<int>& partitions);
    bool assign_partitions(int partition);

    // init成功后调用，
    // partitions的第二个值（second）用于指定分区的偏移（offset），
    // partitions的第一个值（first）为分区ID。
    bool assign_partitions(const std::vector<std::pair<int, int64_t> >& partitions);
    bool assign_partitions(int partition, int64_t offset);

    // 如果为绑定消费模式，则在 close 之前应调用
    bool unassign_partitions();

    // 函数返回 false 是，还会设置下列输出参数：
    // 1）timeout 如果返回值为 true，则表示消费超时
    // 2）empty 如果返回值为 true，则表示队列空无数据
    bool consume(std::string* log, int timeout_ms=1000, struct MessageInfo* mi=NULL, bool* timeout=NULL, bool* empty=NULL);
    int consume_batch(int batch_size, std::vector<std::string>* logs, int timeout_ms=1000, struct MessageInfo* mi=NULL, bool* timeout=NULL, bool* empty=NULL);

    // 同步阻塞提交
    // 返回RdKafka::ERR_NO_ERROR表示成功，其它出错
    int sync_commit();
    int sync_commit(void* message);

    // 异步非阻塞提交
    // 返回RdKafka::ERR_NO_ERROR表示成功，其它出错
    int async_commit();
    int async_commit(void* message);

    // 取得分区数
    int get_num_partitions(std::string* errmsg=NULL) const;

    // 取得 brokers 列表
    std::string get_broker_list(std::string* errmsg=NULL) const;

private:
    int _consumed_mode;
    std::string _brokers_str;
    std::string _topic_str;
    std::string _group_str;

private:
    mooon::utils::ScopedPtr<RdKafka::Conf> _global_conf;
    mooon::utils::ScopedPtr<RdKafka::Conf> _topic_conf;
    mooon::utils::ScopedPtr<RdKafka::KafkaConsumer> _consumer;
    mooon::utils::ScopedPtr<RdKafka::Topic> _topic;

private:
    mooon::utils::ScopedPtr<RdKafka::EventCb> _event_cb;
    mooon::utils::ScopedPtr<RdKafka::ConsumeCb> _consume_cb;
    mooon::utils::ScopedPtr<RdKafka::RebalanceCb> _rebalance_cb;
    mooon::utils::ScopedPtr<RdKafka::OffsetCommitCb> _offset_commitcb;

private:
    std::string _auto_offset_reset;
};

NET_NAMESPACE_END
#endif // MOOON_HAVE_LIBRDKAFKA
#endif // MOOON_NET_KAFKA_CONSUMER_H
