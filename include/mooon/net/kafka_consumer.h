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
    int64_t offset;
    int64_t timestamp;
    std::string topicname;
};

// 注：一个CKafkaConsumer实例只能消费一个topic，
// 被消息的topic由init成员函数指定。
class CKafkaConsumer
{
public:
    // CKafkaProducer负责event_cb、consume_cb、rebalance_cb和offset_commitcb的销毁。
    //
    // 如果event_cb为空，则使用DefEventImpl作为EventCb
    // 如果consume_cb为空，则使用DefConsumeImpl作为ConsumeImpl，
    // 如果rebalance_cb为空，则使用DefRebalanceImpl作为RebalanceImpl，
    // 如果offset_commitcb为空，则使用DefOffsetCommitImpl作为OffsetCommitImpl。
    CKafkaConsumer(RdKafka::EventCb* event_cb=NULL, RdKafka::ConsumeCb* consume_cb=NULL, RdKafka::RebalanceCb* rebalance_cb=NULL, RdKafka::OffsetCommitCb* offset_commitcb=NULL);
    ~CKafkaConsumer();

    // 设置配置auto.offset.reset的值，
    // set_auto_offset_reset需在init之前调用。
    //
    // str可取值：
    // 1) earliest 当各分区下有已提交的offset时从提交的offset开始消费，无提交的offset时则从头开始消费
    // 2) latest 默认值，当各分区下有已提交的offset时从提交的offset开始消费，无提交的offset时则消费新产生的该分区下的数据
    // 3) none Topic各分区都存在已提交的offset时从offset后开始消费，只要有一个分区不存在已提交的offset则抛出异常
    void set_auto_offset_reset(const std::string& str);

    void close(); // 这个调用最长会阻塞 session.timeout.ms 指定的时长（毫秒）
    bool init(const std::string& brokers, const std::string& topic, const std::string& group, bool enable_rebalance=false, bool enable_auto_commit=true);
    bool consume(std::string* log, int timeout_ms=1000, struct MessageInfo* mi=NULL);
    int consume_batch(int batch_size, std::vector<std::string>* logs, int timeout_ms=1000, struct MessageInfo* mi=NULL);

    // 同步阻塞提交
    // 返回RdKafka::ERR_NO_ERROR表示成功，其它出错
    int sync_commit();

    // 异步非阻塞提交
    // 返回RdKafka::ERR_NO_ERROR表示成功，其它出错
    int async_commit();

    // 取得分区数
    int get_num_partitions(std::string* errmsg=NULL) const;

    // 取得 brokers 列表
    std::string get_broker_list(std::string* errmsg=NULL) const;

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

private:
    std::string _auto_offset_reset;
};

NET_NAMESPACE_END
#endif // MOOON_HAVE_LIBRDKAFKA
#endif // MOOON_NET_KAFKA_CONSUMER_H
