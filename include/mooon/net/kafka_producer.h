// Writed by yijian on 2018/11/18
#ifndef MOOON_NET_KAFKA_PRODUCER_H
#define MOOON_NET_KAFKA_PRODUCER_H
#if MOOON_HAVE_LIBRDKAFKA==1 // 宏MOOON_HAVE_LIBRDKAFKA的值须为1
#include <mooon/net/config.h>
#include <mooon/sys/log.h>
#include <mooon/utils/scoped_ptr.h>
#include <librdkafka/rdkafkacpp.h>
NET_NAMESPACE_BEGIN

class DefDeliveryReportImpl;
class DefEventImpl;
class DefPartitionerImpl;

// 非线程安全
// 一个CKafkaProducer只能处理一个topic
class CKafkaProducer
{
public:
    // CKafkaProducer负责dr_cb、event_cb和partitioner_cb的销毁。
    //
    // 如果dr_cb为空，则使用DefDeliveryReportImpl作为DeliveryReportCb，每个消息递送成功或失败（如超时）均会调用一次dr_cb，
    // 如果event_cb为空，则使用DefEventImpl作为EventCb，
    // 如果partitioner_cb为空，则使用DefPartitionerImpl作为PartitionerCb。
    CKafkaProducer(RdKafka::DeliveryReportCb* dr_cb=NULL, RdKafka::EventCb* event_cb=NULL, RdKafka::PartitionerCb* partitioner_cb=NULL);
    bool init(const std::string& brokers_str, const std::string& topic_str, std::string* errmsg=NULL);

    // 调用produce并不一定立即发送消息，而是将消息放到发送队列中缓存，
    // 发送队列大小由配置queue.buffering.max.message指定，如果队列满应调用poll去触发消息发送。
    //
    // 返回true表示成功
    // 返回false为出错，可通过errcode和errmsg得到出错信息
    //
    // errcode取值：
    // 1) RdKafka::ERR_MSG_SIZE_TOO_LARGE 消息字节数超过配置messages.max.bytes指定的值
    // 2) RdKafka::ERR__QUEUE_FULL 发送队列满（达到配置queue.buffering.max.message指定的值）
    // 3) RdKafka::ERR__UNKNOWN_PARTITION 未知分区
    // 4) RdKafka::ERR__UNKNOWN_TOPIC 未知主题
    //
    // 入队有两种方式：
    // 1) 非阻塞方式，如果队列满返回错误代码RD_KAFKA_RESP_ERR__QUEUE_FULL
    // 2) 阻塞方式
    bool produce(const std::string& key, const std::string& log, int32_t partition=RdKafka::Topic::PARTITION_UA, int* errcode=NULL, std::string* errmsg=NULL);

    // errcode和errmsg存储最近一次的出错信息
    // 返回实际数，为0表示一个也没有
    int produce_batch(const std::string& key, const std::vector<std::string>& logs, int32_t partition=RdKafka::Topic::PARTITION_UA, int* errcode=NULL, std::string* errmsg=NULL);

    // librdkafka要求定时调用timed_poll，
    // 否则事件不会被回调，消息会被积压（发送队列满，但因为没调用poll，即使数据已发送出去，然而发送队列的状态值没有改变）。
    // 返回回调的次数（RdKafka::DeliveryCb、RdKafka::EventCb）。
    int timed_poll(int timeout_ms=0);

    // 通过调用timed_poll等待所有的发送完成，直到达到timeout_ms指定的超时时长
    // 返回RdKafka::ERR__TIMED_OUT表示达到timeout_ms指定的超时时长
    // 会触发回调
    int flush(int timeout_ms);

private:
    std::string _brokers_str;
    std::string _topic_str;

private:
    mooon::utils::ScopedPtr<RdKafka::Conf> _global_conf;
    mooon::utils::ScopedPtr<RdKafka::Conf> _topic_conf;
    mooon::utils::ScopedPtr<RdKafka::Producer> _producer;
    mooon::utils::ScopedPtr<RdKafka::Topic> _topic;

private:
    mooon::utils::ScopedPtr<RdKafka::DeliveryReportCb> _dr_cb;
    mooon::utils::ScopedPtr<RdKafka::EventCb> _event_cb;
    mooon::utils::ScopedPtr<RdKafka::PartitionerCb> _partitioner_cb;
};

NET_NAMESPACE_END
#endif // MOOON_HAVE_LIBRDKAFKA
#endif // MOOON_NET_KAFKA_PRODUCER_H
