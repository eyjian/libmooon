// Writed by yijian on 2019/8/5
#include "net/kafka_consumer.h"
#include "sys/datetime_utils.h"
#include "sys/log.h"
#include "utils/string_utils.h"
#include <time.h>
#if MOOON_HAVE_LIBRDKAFKA==1
NET_NAMESPACE_BEGIN

// 消息模式
enum ConsumedMode
{
    CM_NONE, // 未指定消费模式
    CM_SUBSCRIBE, // 订阅消费模式
    CM_ASSIGN // 绑定分区消费模式
};

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

class DefOffsetCommitImpl: public RdKafka::OffsetCommitCb
{
private:
    // 由RdKafka::KafkaConsumer::consume()触发调用
    virtual void offset_commit_cb(RdKafka::ErrorCode err, std::vector<RdKafka::TopicPartition*>& offsets)
    {
        if (MYLOG_DEBUG_ENABLE())
        {
            for (int i=0; i<int(offsets.size()); ++i)
            {
                MYLOG_DEBUG("topic://%s, partition:%d, offset:%" PRId64", errcode:%d/%d\n",
                        offsets[i]->topic().c_str(), offsets[i]->partition(), offsets[i]->offset(), (int)offsets[i]->err(), err);
            }
        }
    }
};

// CKafkaConsumer

CKafkaConsumer::CKafkaConsumer(RdKafka::EventCb* event_cb, RdKafka::ConsumeCb* consume_cb, RdKafka::RebalanceCb* rebalance_cb, RdKafka::OffsetCommitCb* offset_commitcb)
{
    _consumed_mode = CM_NONE;

    // KafkaConsumer::create和Producer::create
    // 调用HandleImpl::set_common_config注册下列回调。

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

    // offset_commitcb
    if (NULL == offset_commitcb)
        _offset_commitcb.reset(new DefOffsetCommitImpl);
    else
        _offset_commitcb.reset(offset_commitcb);
}

CKafkaConsumer::~CKafkaConsumer()
{
    bool success = true;
    if (_consumed_mode == CM_SUBSCRIBE)
        success = unsubscribe_topic();
    else if (_consumed_mode == CM_ASSIGN)
        success = unassign_partitions();
    else
        success = true;
    if (success)
        close();
}

void CKafkaConsumer::set_auto_offset_reset(const std::string& str)
{
    _auto_offset_reset = str;
}

void CKafkaConsumer::close()
{
    // Close and shut down the proper
    // 最大阻塞时间由配置 session.timeout.ms 指定
    // 过程中 RdKafka::RebalanceCb 和 RdKafka::OffsetCommitCb 可能被调用
    if (_consumer.get() != NULL)
    {
        // The consumer object must later be freed with delete
        const RdKafka::ErrorCode errcode = _consumer->close();
        if (errcode != RdKafka::ERR_NO_ERROR)
        {
            MYLOG_ERROR("Close topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, err2str(errcode).c_str());
        }
        else
        {
            _consumer.reset(NULL);
        }
    }
}

bool CKafkaConsumer::init(const std::string& brokers, const std::string& topic, const std::string& group, bool enable_rebalance, bool enable_auto_commit)
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

        // enable.auto.commit
        if (enable_auto_commit)
        {
            ret = _global_conf->set("enable.auto.commit", "true", errmsg);
        }
        else
        {
            ret = _global_conf->set("enable.auto.commit", "false", errmsg);
        }
        if (ret != RdKafka::Conf::CONF_OK)
        {
            MYLOG_ERROR("Set enable.auto.commit error: %s.\n", errmsg.c_str());
            break;
        }

        if (!_auto_offset_reset.empty())
        {
            const int ret = _global_conf->set("auto.offset.reset", _auto_offset_reset, errmsg);
            if (ret != RdKafka::Conf::CONF_OK)
            {
                MYLOG_ERROR("Set auto.offset.reset error: %s.\n", errmsg.c_str());
                break;
            }
        }

        // Create consumer
        // 在Producer::create中完成回调注册
        _consumer.reset(RdKafka::KafkaConsumer::create(_global_conf.get(), errmsg));
        if (_consumer.get() == NULL)
        {
            MYLOG_ERROR("Create kafka consumer failed: %s.\n", errmsg.c_str());
            break;
        }

        MYLOG_INFO("Create consumer for topic://%s successfully.\n", topic.c_str());
        _brokers_str = brokers;
        _topic_str = topic;
        _group_str = group;
        return true;
    } while (false);

    return false;
}

bool CKafkaConsumer::subscribe_topic()
{
    std::vector<std::string> topics;
    topics.push_back(_topic_str);

    // Subscribe topics
    const RdKafka::ErrorCode errcode = _consumer->subscribe(topics);
    if (errcode == RdKafka::ERR_NO_ERROR)
    {
        _consumed_mode = CM_SUBSCRIBE;
        return true;
    }
    else
    {
        MYLOG_ERROR("Subscribe topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, err2str(errcode).c_str());
        return false;
    }
}

bool CKafkaConsumer::unsubscribe_topic()
{
    const RdKafka::ErrorCode errcode = _consumer->unsubscribe();
    if (errcode == RdKafka::ERR_NO_ERROR)
    {
        _consumed_mode = CM_NONE;
        return true;
    }
    else
    {
        MYLOG_ERROR("Unsubscribe topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, err2str(errcode).c_str());
        return false;
    }
}

bool CKafkaConsumer::assign_partitions(const std::vector<int>& partitions)
{
    std::vector<RdKafka::TopicPartition*> topic_partitions;
    for (std::vector<int>::size_type i=0; i<partitions.size(); ++i)
    {
        const int partition = partitions[i];
        RdKafka::TopicPartition* topic_partition = RdKafka::TopicPartition::create(_topic_str, partition);
        topic_partitions.push_back(topic_partition);
    }
    const RdKafka::ErrorCode errcode = _consumer->assign(topic_partitions);
    if (errcode == RdKafka::ERR_NO_ERROR)
    {
        _consumed_mode = CM_ASSIGN;
        return true;
    }
    else
    {
        MYLOG_ERROR("Assign topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, err2str(errcode).c_str());
        return false;
    }
}

bool CKafkaConsumer::assign_partitions(int partition)
{
    std::vector<int> partitions(1);
    partitions[0] = partition;
    return assign_partitions(partitions);
}

bool CKafkaConsumer::assign_partitions(const std::vector<std::pair<int, int64_t> >& partitions)
{
    std::vector<RdKafka::TopicPartition*> topic_partitions;
    for (std::vector<int>::size_type i=0; i<partitions.size(); ++i)
    {
        const int partition = partitions[i].first;
        const int64_t offset = partitions[i].second;
        RdKafka::TopicPartition* topic_partition = RdKafka::TopicPartition::create(_topic_str, partition, offset);
        topic_partitions.push_back(topic_partition);
    }
    const RdKafka::ErrorCode errcode = _consumer->assign(topic_partitions);
    if (errcode == RdKafka::ERR_NO_ERROR)
    {
        _consumed_mode = CM_ASSIGN;
        return true;
    }
    else
    {
        MYLOG_ERROR("Assign topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, err2str(errcode).c_str());
        return false;
    }
}

bool CKafkaConsumer::assign_partitions(int partition, int64_t offset)
{
    std::vector<std::pair<int, int64_t> > partitions(1);
    partitions[0].first = partition;
    partitions[0].second = offset;
    return assign_partitions(partitions);
}

bool CKafkaConsumer::unassign_partitions()
{
    const RdKafka::ErrorCode errcode = _consumer->unassign();
    if (errcode == RdKafka::ERR_NO_ERROR)
    {
        _consumed_mode = CM_NONE;
        return true;
    }
    else
    {
        MYLOG_ERROR("Unassign topic://%s error: (%d)%s.\n", _topic_str.c_str(), (int)errcode, err2str(errcode).c_str());
        return false;
    }
}

bool CKafkaConsumer::consume(std::string* log, int timeout_ms, struct MessageInfo* mi, bool* timeout)
{
    // 注意对于消费者，不能调用poll，请参见rdkafkacpp.h中对函数consume的说明
    const RdKafka::Message* message = _consumer->consume(timeout_ms);
    const RdKafka::ErrorCode errcode = message->err();

    if (timeout != NULL)
    {
        *timeout = false;
    }
    if (RdKafka::ERR_NO_ERROR == errcode)
    {
        MYLOG_DEBUG("Consume topic://%s OK: %.*s.\n", _topic_str.c_str(), (int)message->len(), (char*)message->payload());
        log->assign(reinterpret_cast<char*>(message->payload()), message->len());
        if (mi != NULL) {
            mi->offset = message->offset();
            mi->timestamp = message->timestamp().timestamp;
            mi->topicname = message->topic_name();
        }
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
        if (RdKafka::ERR__TIMED_OUT == errcode && timeout != NULL)
        {
            *timeout = true;
        }

        delete message;
        return false;
    }
}

int CKafkaConsumer::consume_batch(int batch_size, std::vector<std::string>* logs, int timeout_ms, struct MessageInfo* mi, bool* timeout)
{
    const int64_t end = int64_t(sys::CDatetimeUtils::get_current_milliseconds() + timeout_ms);
    int64_t remaining_timeout = timeout_ms;
    int num_logs  = 0;
    logs->reserve(batch_size);

    *timeout = false;
    for (int i=0; i<batch_size; ++i)
    {
        // 注意对于消费者，不能调用poll，请参见rdkafkacpp.h中对函数consume的说明
        const RdKafka::Message* message = _consumer->consume(remaining_timeout);
        const RdKafka::ErrorCode errcode = message->err();

        if (RdKafka::ERR_NO_ERROR == errcode)
        {
            MYLOG_DEBUG("Consume topic://%s OK: %.*s.\n", _topic_str.c_str(), (int)message->len(), (char*)message->payload());

            const std::string log(reinterpret_cast<char*>(message->payload()), message->len());
            logs->push_back(log);
            if (mi != NULL)
            {
                mi->offset = message->offset();
                mi->timestamp = message->timestamp().timestamp;
                mi->topicname = message->topic_name();
            }

            delete message;
            ++num_logs;
            remaining_timeout = end - int64_t(sys::CDatetimeUtils::get_current_milliseconds());
            if (remaining_timeout <= 0)
                break;
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
            if (RdKafka::ERR__TIMED_OUT == errcode && num_logs == 0)
                *timeout = true;
            break;
        }
    }

    return num_logs;
}

int CKafkaConsumer::sync_commit()
{
    return int(_consumer->commitSync());
}

int CKafkaConsumer::async_commit()
{
    return int(_consumer->commitAsync());
}

// 参考：rdkafka_example.cpp
int CKafkaConsumer::get_num_partitions(std::string* errmsg) const
{
    const int timeout_ms = 2000;
    RdKafka::Topic *topic = NULL;
    class RdKafka::Metadata *metadata;
    int num_partitions = -1;

    RdKafka::ErrorCode err = _consumer->metadata(false, topic, &metadata, timeout_ms);
    if (err != RdKafka::ERR_NO_ERROR)
    {
        num_partitions = -1;
        if (errmsg != NULL)
            *errmsg = RdKafka::err2str(err);
    }
    else
    {
        // 遍历所有 topics
        for (RdKafka::Metadata::TopicMetadataIterator it = metadata->topics()->begin(); it != metadata->topics()->end(); ++it)
        {
            // 目标 topic
            if ((*it)->topic() == _topic_str)
            {
                num_partitions = (*it)->partitions()->size();
                break;
            }
        }
    }

    return num_partitions;
}

// 参考：rdkafka_example.cpp:metadata_print()
std::string CKafkaConsumer::get_broker_list(std::string* errmsg) const
{
    const int timeout_ms = 2000;
    RdKafka::Topic *topic = NULL;
    class RdKafka::Metadata *metadata;
    std::string broker_list;

    RdKafka::ErrorCode err = _consumer->metadata(false, topic, &metadata, timeout_ms);
    if (err != RdKafka::ERR_NO_ERROR)
    {
        if (errmsg != NULL)
            *errmsg = RdKafka::err2str(err);
    }
    else
    {
        // brokers 个数：metadata->brokers()->size()
        for (RdKafka::Metadata::BrokerMetadataIterator ib=metadata->brokers()->begin(); ib != metadata->brokers()->end(); ++ib)
        {
            // (*ib)->id()
            if (broker_list.empty())
                broker_list = mooon::utils::CStringUtils::format_string("%s:%d", (*ib)->host().c_str(), (*ib)->port());
            else
                broker_list = mooon::utils::CStringUtils::format_string("%s:%d,%s", (*ib)->host().c_str(), (*ib)->port(), broker_list.c_str());
        }
    }

    return broker_list;
}

NET_NAMESPACE_END
#endif // MOOON_HAVE_LIBRDKAFKA
