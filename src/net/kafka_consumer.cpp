// Writed by yijian on 2019/8/5
#include "net/kafka_consumer.h"
#include "sys/log.h"
#if MOOON_HAVE_LIBRDKAFKA==1
NET_NAMESPACE_BEGIN

class DefEventImpl: public RdKafka::EventCb
{
private:
    virtual void event_cb (RdKafka::Event &event)
    {
        MYLOG_DEBUG("EventCb: type(%d), severity(%d), (%d)%s\n",
                event.type(), event.severity(), event.err(), event.str().c_str());
    }
};

class DefConsumeImpl: public RdKafka::ConsumeCb
{
private:
    virtual void consume_cb (RdKafka::Message &message, void *opaque)
    {
        MYLOG_DEBUG("ConsumeCb: (%d)%s, %.*s\n",
                message.err(), message.errstr().c_str(), (int)message.len(), (char*)message.payload());
    }
};

class DefRebalanceImpl: public RdKafka::RebalanceCb
{
private:
    virtual void rebalance_cb (RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode errcode, std::vector<RdKafka::TopicPartition*>& partitions)
    {
        if (RdKafka::ERR__ASSIGN_PARTITIONS == errcode)
        {
            // Application may load offets from arbitrary external storage here and update partitions
            MYLOG_DEBUG("RebalanceCb: (%d)\n", (int)errcode);
            consumer->assign(partitions);
        }
        else if (RdKafka::ERR__REVOKE_PARTITIONS == errcode)
        {
            // Application may commit offsets manually here if auto.commit.enable=false
            consumer->unassign();
        }
        else
        {
            MYLOG_ERROR("RebalanceCb error: %s\n", RdKafka::err2str(errcode).c_str());
            consumer->unassign();
        }
    }
};

CKafkaConsumer::CKafkaConsumer(RdKafka::EventCb* event_cb, RdKafka::ConsumeCb* consume_cb, RdKafka::RebalanceCb* rebalance_cb)
{
    // event_cb
    if (NULL == event_cb)
        _event_cb.reset(new DefEventImpl);
    else
        _event_cb.reset(event_cb);

    // consume_cb
    if (NULL == consume_cb)
        _consume_cb.reset(new DefConsumeImpl);
    else
        _consume_cb.reset(consume_cb);

    // rebalance_cb
    if (NULL == rebalance_cb)
        _rebalance_cb.reset(new DefRebalanceImpl);
    else
        _rebalance_cb.reset(rebalance_cb);
}

CKafkaConsumer::~CKafkaConsumer()
{
    _consumer->close();
}

bool CKafkaConsumer::init(const std::string& brokers, const std::string& topic, const std::string& group, bool enable_rebalance)
{
    if (brokers.empty())
    {
        MYLOG_ERROR("Parameter[brokers] is empty.\n");
        return false;
    }
    if (topic.empty())
    {
        MYLOG_ERROR("Parameter[topic] is empty.\n");
        return false;
    }
    if (group.empty())
    {
        MYLOG_ERROR("Parameter[group] is empty.\n");
        return false;
    }
    do
    {
        std::string errmsg;
        std::vector<std::string> topics;
        topics.push_back(topic);
        _global_conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
        _topic_conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));

        // event_cb
        RdKafka::Conf::ConfResult ret = _global_conf->set("event_cb", _event_cb.get(), errmsg);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            MYLOG_ERROR("Set event_cb error: %s.\n", errmsg.c_str());
            break;
        }

        // metadata.broker.list
        ret = _global_conf->set("metadata.broker.list", brokers, errmsg);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            MYLOG_ERROR("Set metadata.broker.list error: %s.\n", errmsg.c_str());
            break;
        }

        // group.id
        ret = _global_conf->set("group.id", group, errmsg);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            MYLOG_ERROR("Set group.id error: %s.\n", errmsg.c_str());
            break;
        }

        // consume_cb
        ret = _global_conf->set("consume_cb", _consume_cb.get(), errmsg);
        if (ret != RdKafka::Conf::CONF_OK)
        {
            MYLOG_ERROR("Set consume_cb error: %s.\n", errmsg.c_str());
            break;
        }

        // rebalance_cb
        if (enable_rebalance)
        {
            ret = _global_conf->set("rebalance_cb", _rebalance_cb.get(), errmsg);
            if (ret != RdKafka::Conf::CONF_OK)
            {
                MYLOG_ERROR("Set rebalance_cb error: %s.\n", errmsg.c_str());
                break;
            }
        }

        // Create consumer
        _consumer.reset(RdKafka::KafkaConsumer::create(_global_conf.get(), errmsg));
        if (_consumer.get() == NULL)
        {
            MYLOG_ERROR("Create kafka consumer failed: %s.\n", errmsg.c_str());
            break;
        }

        // Subscribe topics
        const RdKafka::ErrorCode errcode = _consumer->subscribe(topics);
        if (errcode != RdKafka::ERR_NO_ERROR)
        {
            MYLOG_ERROR("Subscribe topic://%s error: (%d)%s.\n", topic.c_str(), (int)errcode, err2str(errcode).c_str());
            break;
        }

        MYLOG_INFO("Subscribe topic://%s OK.\n", topic.c_str());
        _brokers_str = brokers;
        _topic_str = topic;
        _group_str = group;
        return true;
    } while (false);

    return false;
}

bool CKafkaConsumer::consume(std::string* log, int timeout_ms)
{
    const RdKafka::Message* message = _consumer->consume(timeout_ms);
    const RdKafka::ErrorCode errcode = message->err();

    if (RdKafka::ERR_NO_ERROR == errcode)
    {
        MYLOG_DEBUG("Consume topic://%s OK: %.*s.\n", _topic_str.c_str(), (int)message->len(), (char*)message->payload());
        log->assign(reinterpret_cast<char*>(message->payload()), message->len());
        delete message;
        return true;
    }
    else
    {
        // (ERR__PARTITION_EOF, -191)Broker: No more messages
        // (ERR__TIMED_OUT, -185)Local: Timed out
        if ((RdKafka::ERR__TIMED_OUT == errcode) ||
            (RdKafka::ERR__PARTITION_EOF == errcode))
        {
            MYLOG_DETAIL("Consume topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, message->errstr().c_str());
        }
        else
        {
            MYLOG_ERROR("Consume topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, message->errstr().c_str());
        }

        delete message;
        return false;
    }
}

NET_NAMESPACE_END
#endif // MOOON_HAVE_LIBRDKAFKA
