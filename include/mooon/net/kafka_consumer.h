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

struct MessageInfo
{
    int64_t offset;
    int64_t timestamp;
    std::string topicname;
};

// 注：一个CKafkaConsumer实例只能消费一个topic，
// 被消息的topic由init成员函数指定。
class CKafkaConsumer
{
public:
    // CKafkaProducer负责event_cb、consume_cb和rebalance_cb的销毁。
    //
    // 如果event_cb为空，则使用DefEventImpl作为EventCb
    // 如果consume_cb为空，则使用DefConsumeImpl作为ConsumeImpl，
    // 如果rebalance_cb为空，则使用DefRebalanceImpl作为RebalanceImpl
    CKafkaConsumer(RdKafka::EventCb* event_cb=NULL, RdKafka::ConsumeCb* consume_cb=NULL, RdKafka::RebalanceCb* rebalance_cb=NULL, RdKafka::OffsetCommitCb* offset_commitcb=NULL);
    ~CKafkaConsumer();
    bool init(const std::string& brokers, const std::string& topic, const std::string& group, bool enable_rebalance=false);
    bool consume(std::string* log, int timeout_ms=1000, struct MessageInfo* mi=NULL);
    int consume_batch(int batch_size, std::vector<std::string>* logs, int timeout_ms=1000, struct MessageInfo* mi=NULL);

    // 同步阻塞提交
    // 返回RdKafka::ERR_NO_ERROR表示成功，其它出错
    int sync_commit();

    // 异步非阻塞调用
    // 返回RdKafka::ERR_NO_ERROR表示成功，其它出错
    int async_commit();

private:
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
};

NET_NAMESPACE_END
#endif // MOOON_HAVE_LIBRDKAFKA
#endif // MOOON_NET_KAFKA_CONSUMER_H
