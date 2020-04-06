// Writed by yijian on 2018/11/18
#include "net/kafka_producer.h"
#include "utils/string_utils.h"
#if MOOON_HAVE_LIBRDKAFKA==1
NET_NAMESPACE_BEGIN

class DefDeliveryReportImpl: public RdKafka::DeliveryReportCb
{
public:
    virtual ~DefDeliveryReportImpl() {}

private:
    virtual void dr_cb (RdKafka::Message &message);
};

class DefEventImpl: public RdKafka::EventCb
{
public:
    virtual ~DefEventImpl() {}

private:
    virtual void event_cb (RdKafka::Event &event);
};

class DefPartitionerImpl: public RdKafka::PartitionerCb
{
public:
    virtual ~DefPartitionerImpl() {}

private:
    virtual int32_t partitioner_cb (const RdKafka::Topic *topic, const std::string *key, int32_t partition_cnt, void *msg_opaque);
};

void DefDeliveryReportImpl::dr_cb (RdKafka::Message &message)
{
    // delivery_callback: (-192)Local: Message timed out
    // 错误原因：
    // 运行librdkafka的程序所在机器没有设置Kafka broker的hosts，
    // 也就是通过hostname找不到对应的IP地址。
    const RdKafka::ErrorCode errcode = message.err();
    const std::string& errmsg = message.errstr();
    if (RdKafka::ERR_NO_ERROR == errcode)
        MYLOG_DEBUG("delivery_callback: (%d)%s\n", errcode, errmsg.c_str());
    else
        MYLOG_ERROR("delivery_callback: (%d)%s\n", errcode, errmsg.c_str());
}

void DefEventImpl::event_cb (RdKafka::Event &event)
{
    MYLOG_DEBUG("event_callback: type(%d), severity(%d), (%d)%s\n", event.type(), event.severity(), event.err(), event.str().c_str());
}

int32_t DefPartitionerImpl::partitioner_cb (const RdKafka::Topic *topic, const std::string *key, int32_t partition_cnt, void *msg_opaque)
{
    const uint32_t hash = mooon::utils::CStringUtils::hash(key->c_str(), static_cast<int>(key->size()));
    return static_cast<int>(hash % partition_cnt);
}

// CKafkaProducer

CKafkaProducer::CKafkaProducer(RdKafka::DeliveryReportCb* dr_cb, RdKafka::EventCb* event_cb, RdKafka::PartitionerCb* partitioner_cb)
{
    // KafkaConsumer::create和Producer::create
    // 调用HandleImpl::set_common_config注册下列回调。

    // dr_cb
    if (NULL == dr_cb)
        _dr_cb.reset(new DefDeliveryReportImpl);
    else
        _dr_cb.reset(dr_cb);

    // event_cb
    if (NULL == event_cb)
        _event_cb.reset(new DefEventImpl);
    else
        _event_cb.reset(event_cb);

    // partitioner_cb
    if (NULL == partitioner_cb)
        _partitioner_cb.reset(new DefPartitionerImpl);
    else
        _partitioner_cb.reset(partitioner_cb);
}

bool CKafkaProducer::init(const std::string& brokers_str, const std::string& topic_str, std::string* errmsg)
{
    _brokers_str = brokers_str;
    _topic_str = topic_str;

    if (_brokers_str.empty())
    {
        if (errmsg != NULL)
            *errmsg = "no any broker specified";
        return false;
    }
    if (_topic_str.empty())
    {
        if (errmsg != NULL)
            *errmsg = "no topic specified";
        return false;
    }

    do
    {
        std::string errmsg_;
        RdKafka::Conf::ConfResult ret;
        _global_conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
        _topic_conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));

        // request.required.acks
        //ret = _global_conf->set("request.required.acks", "1", errmsg_);

        // metadata.broker.list
        ret = _global_conf->set("metadata.broker.list", _brokers_str, errmsg_);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            if (errmsg != NULL)
                *errmsg = mooon::utils::CStringUtils::format_string("set metadata.broker.list://%s error: %s", _brokers_str.c_str(), errmsg_.c_str());
            break;
        }

        // dr_cb
        ret = _global_conf->set("dr_cb", _dr_cb.get(), errmsg_);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            if (errmsg != NULL)
                *errmsg = mooon::utils::CStringUtils::format_string("set dr_cb error: %s", errmsg_.c_str());
            break;
        }

        // event_cb
        ret = _global_conf->set("event_cb", _event_cb.get(), errmsg_);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            if (errmsg != NULL)
                *errmsg = mooon::utils::CStringUtils::format_string("set event_cb error: %s", errmsg_.c_str());
            break;
        }

        // partitioner_cb
        ret = _topic_conf->set("partitioner_cb", _partitioner_cb.get(), errmsg_);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            if (errmsg != NULL)
                *errmsg = mooon::utils::CStringUtils::format_string("set partitioner_cb error: %s", errmsg_.c_str());
            break;
        }

        // producer
        // 在Producer::create中完成回调注册
        _producer.reset(RdKafka::Producer::create(_global_conf.get(), errmsg_));
        if (NULL == _producer.get())
        {
            if (errmsg != NULL)
                *errmsg = mooon::utils::CStringUtils::format_string("create producer failed: %s", errmsg_.c_str());
            break;
        }

        // topic
        _topic.reset(RdKafka::Topic::create(_producer.get(), _topic_str, _topic_conf.get(), errmsg_));
        if (NULL == _topic.get())
        {
            if (errmsg != NULL)
                *errmsg = mooon::utils::CStringUtils::format_string("create topic://%s failed: %s", _topic_str.c_str(), errmsg_.c_str());
            break;
        }

        return true;
    } while(false);

    return false;
}

bool CKafkaProducer::produce(const std::string& key, const std::string& log, int32_t partition, int* errcode, std::string* errmsg)
{
    const RdKafka::ErrorCode errcode_ = _producer->produce(_topic.get(), partition, RdKafka::Producer::RK_MSG_COPY,  (void*)log.data(), log.size(), &key, NULL);
    timed_poll(0);

    // log可能是二进制数据，这里无法解析，所以并不适合记录到日志文件中
    if (RdKafka::ERR_NO_ERROR == errcode_)
    {
        if (errcode != NULL)
            *errcode = 0;
        if (errmsg != NULL)
            *errmsg = "SUCCESS";

        return true;
    }
    else
    {
        if (errcode != NULL)
            *errcode = errcode_;
        if (errmsg != NULL)
            *errmsg = err2str(errcode_);
        return false;
    }
}

int CKafkaProducer::produce_batch(const std::string& key, const std::vector<std::string>& logs, int32_t partition, int* errcode, std::string* errmsg)
{
    int num_logs = 0;

    timed_poll(0);
    for (int i=0; i<int(logs.size()); ++i)
    {
        const std::string& log = logs[i];
        // 入队（发送队列）有两种方式：
        // 1) 非阻塞方式，如果队列满返回错误代码RD_KAFKA_RESP_ERR__QUEUE_FULL
        // 2) 阻塞方式，需指定RdKafka::RK_MSG_BLOCK
        const RdKafka::ErrorCode errcode_ = _producer->produce(_topic.get(), partition, RdKafka::Producer::RK_MSG_COPY,  (void*)log.data(), log.size(), &key, NULL);

        if (RdKafka::ERR_NO_ERROR == errcode_)
        {
            ++num_logs;
            if (errcode != NULL)
                *errcode = 0;
            if (errmsg != NULL)
                *errmsg = "SUCCESS";
        }
        else
        {
            if (errcode != NULL)
                *errcode = errcode_;
            if (errmsg != NULL)
                *errmsg = err2str(errcode_);
            if (errcode_ == RdKafka::ERR__QUEUE_FULL)
                timed_poll(0);
            break;
        }
    }


    return num_logs;
}

// rd_kafka_poll_cb:
// int rd_kafka_poll (rd_kafka_t *rk, int timeout_ms) {
//        return rd_kafka_q_serve(rk->rk_rep, timeout_ms, 0,
//                                RD_KAFKA_Q_CB_CALLBACK, rd_kafka_poll_cb, NULL);
// }
//
// rd_kafka_op_handle/rdkafka_op.c
// {
//   if (callback)
//   {
//     rd_kafka_op_res_t res = callback(rk, rkq, rko, cb_type, opaque);
//   }
// }
int CKafkaProducer::timed_poll(int timeout_ms)
{
    // Polls the provided kafka handle for events
    // poll returns the number of events served
    // poll只针对生产者：
    // （rk->rk_type != RD_KAFKA_PRODUCER => RD_KAFKA_RESP_ERR__NOT_IMPLEMENTED）
    //
    // HandleImpl::poll/rdkafkacpp_int.h -> rd_kafka_poll/rdkafka.c
    //
    // 这里的timeout实际是一个pthread_cond_timedwait或pthread_cond_wait操作，
    // 相关数据结构为rd_kafka_q_s::rkq_cond，谁cnd_signal（唤醒）它了？
    //
    // 实际是检测队列rd_kafka_q_s::rkq_q，如果没有数据则等待timeout_ms时。
    //
    // rko: rd_kafka_op_t
    // rkq: rd_kafka_q_t
    //
    // rd_kafka_q_enq1:
    // 将rko压入rkq的队首或队尾。
    //
    // rd_kafka_q_enq: 将rko放入队尾
    // rd_kafka_q_reenq: 将rko重新放到
    //
    // rd_kafka_q_enq/rdkafka_queue.h
    // -> rd_kafka_q_enq1/rdkafka_queue.h(Enqueue rko either at head or tail of rkq)
    // -> rd_kafka_q_enq0(Low-level unprotected enqueue)
    return _producer->poll(timeout_ms);
}

int CKafkaProducer::flush(int timeout_ms)
{
    // Producer::poll/rdkafkacpp.h
    // -> ProducerImpl::rd_kafka_poll/rdkafkacpp_int.h
    // -> rd_kafka_flush -> rd_kafka_poll/rdkafka.c
    // -> rd_kafka_q_serve/rdkafka_queue.c -> rd_kafka_op_handle/rdkafka_op.c
    // -> callback(rd_kafka_q_serve_cb_t)/rdkafka_op.h
    return _producer->flush(timeout_ms);
}

// kafka_transport.c:
// 生产者和消费者网络IO，独立网络线程，和工作线程间以队列交互。
// 进程启动时创建broker网络线程：thrd_create(&rkb->rkb_thread, rd_kafka_broker_thread_main, rkb)
// -> rd_kafka_broker_thread_main
// -> [*此处循环poll*]rd_kafka_broker_producer_serve[rd_kafka_broker_consumer_serve]
// -> rd_kafka_broker_serve
// -> rd_kafka_transport_io_serve (先-> rd_kafka_transport_poll -> poll)
// -> 后rd_kafka_transport_io_event
// -> rd_kafka_recv
// -> rd_kafka_transport_recv
// -> rd_kafka_transport_socket_recv
// -> rd_kafka_transport_socket_recv0
// -> recv/最底层的系统调用（接收缓冲区：rktrans_s）
//
// librdkafka并没有使用epoll，而是使用poll，见rd_kafka_transport_poll的实现。
// 实际上一个线程只poll了两个fd，所以用不上epoll，直接用poll足够了。
//
// 这些线程（rd_kafka_broker_thread_main）是在调用KafkaConsumer::create和Producer::create时创建，
// 会为每个broker均创建一个rd_kafka_broker_thread_main线程。

NET_NAMESPACE_END
#endif // MOOON_HAVE_LIBRDKAFKA
