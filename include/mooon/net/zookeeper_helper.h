/**
 * Copyright (c) 2016, Jian Yi <eyjian at gmail dot com>
 *
 * All rights reserved.
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MOOON_NET_ZOOKEEPER_HELPER_H
#define MOOON_NET_ZOOKEEPER_HELPER_H
#include <mooon/net/config.h>
#include <mooon/net/utils.h>
#include <mooon/sys/utils.h>
#include <mooon/utils/exception.h>
#include <mooon/utils/string_utils.h>

#if MOOON_HAVE_ZOOKEEPER == 1
#include <zookeeper/zookeeper.h>
#endif // MOOON_HAVE_ZOOKEEPER

NET_NAMESPACE_BEGIN
#if MOOON_HAVE_ZOOKEEPER == 1

// 默认zookeeper日志输出到stderr，
// 可以调用zoo_set_log_stream(FILE*)设置输出到文件中
// 还可以调用zoo_set_debug_level(ZooLogLevel)控制日志级别！！！

// 提供基于zookeeper的主备切换接口和读取数据等接口
//
// 使用示例：
//
class CZookeeperHelper
{
public:
    static bool node_exists_exception(int errcode) { return ZNODEEXISTS == errcode; }
    static bool node_not_exists_exception(int errcode) { return ZNONODE == errcode; }
    static bool invalid_state_exception(int errcode) { return ZINVALIDSTATE == errcode; }
    static bool connection_loss_exception(int errcode) { return ZCONNECTIONLOSS == errcode; }
    static bool not_empty_exception(int errcode) { return ZNOTEMPTY == errcode; }
    static bool session_expired_exception(int errcode) { return ZSESSIONEXPIRED == errcode; }
    static bool no_authorization_exception(int errcode) { return ZNOAUTH == errcode; }
    static bool invalid_ACL_exception(int errcode) { return ZINVALIDACL == errcode; }

public:
    CZookeeperHelper() throw ();
    virtual ~CZookeeperHelper();

    // 取zk_path的数据
    //
    // zk_data 存放存储在path的数据
    // data_size 最多获取的数据大小，注意数据的实际大小可能比data_size指定的值大
    // keep_watch 是否保持watch该path
    //
    // 如果成功返回数据实际大小，如果出错则抛异常
    int get_zk_data(const char* zk_path, std::string* zk_data, int data_size=SIZE_4K, bool keep_watch=true) const throw (utils::CException);
    std::string get_zk_data(const char* zk_path, int data_size=SIZE_4K, bool keep_watch=true) const throw (utils::CException);

    // 取函数connect_zookeeper的参数指定的zk_nodes
    const std::string& get_zk_nodes() const throw () { return _zk_nodes; }

    // 建立与zookeepr的连接
    // 但请注意本函数返回并不表示和zookeeper连接成功，
    // 只有成员函数on_zookeeper_connected()被回调了才表示连接成功
    //
    // zk_nodes 以逗号分隔的zookeeper节点，如：192.168.31.239:2181,192.168.31.240:2181
    void connect_zookeeper(const std::string& zk_nodes, int session_timeout_seconds=6) throw (utils::CException);

    // 关闭与zookeeper的连接
    void close_zookeeper() throw (utils::CException);

    // 重新建立与zookeeper的连接，重连接之前会先关闭和释放已建立连接的资源
    //
    // 请注意，
    // 在调用connect_zookeeper()或reconnect_zookeeper()后，
    // 都应当重新调用race_master()去竞争成为master。
    void reconnect_zookeeper() throw (utils::CException);

    // 得到当前连接的zookeeper host
    std::string get_connected_host() const throw ();
    bool get_connected_host(std::string* ip, uint16_t* port) const throw ();

    // 返回当前是否处于连接状态
    bool is_connected() const throw ();

    // 取得实际的zookeeper session超时时长，单位为秒
    int get_session_timeout_seconds() const;

    // 竞争成为master
    // 函数is_connected()返回true，方可调用race_master()
    //
    // zk_path 用于竞争的path，不能为空，且父path必须已经存在
    // path_data 存储到zk_path的数据，用于区分主备，因此主备的path_data不能相同。
    // zk_errcode 出错代码，如果zk_errcode的值为-110（）表示已有master，即node_exists_exception(zk_errcode)返回true
    // zk_errmsg 出错信息
    //
    // 由于仅基于zookeeper的ZOO_EPHEMERAL结点实现互斥，没有组合使用ZOO_SEQUENCE
    //
    // 如果竞争成功返回true，否则返回false
    bool race_master(const std::string& zk_path, const std::string& path_data, int* zk_errcode=NULL, std::string* zk_errmsg=NULL) throw ();

    // 创建一个节点
    //
    // zk_parent_path 父节点的路径，值不能以“/”结尾，如果父节点为“/”则值需为空
    //
    // flags可取值：
    // 1) 0 普通节点
    // 2) ZOO_EPHEMERAL 临时节点
    // 3) ZOO_SEQUENCE 顺序节点
    // 4) ZOO_EPHEMERAL|ZOO_SEQUENCE 临时顺序节点
    //
    // acl可取值（如果值为NULL，则相当于取值ZOO_OPEN_ACL_UNSAFE）：
    // 1) ZOO_OPEN_ACL_UNSAFE 全开放，不做权限控制
    // 2) ZOO_READ_ACL_UNSAFE 只读的
    // 3) ZOO_CREATOR_ALL_ACL 创建者拥有所有权限
    void create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data, int flags, const struct ACL_vector *acl) throw (utils::CException);
    void create_node(const std::string& zk_parent_path, const std::string& zk_node_name) throw (utils::CException);
    void create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data) throw (utils::CException);
    void create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data, int flags) throw (utils::CException);
    void create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data, const struct ACL_vector *acl) throw (utils::CException);

    // 删除一个节点
    //
    // version 如果值为-1，则不做版本检查直接删除，否则因版本不致删除失败
    void delete_node(const std::string& zk_path, int version=-1) throw (utils::CException);

public: // 仅局限于被zk_watcher()调用，其它情况均不应当调用
    void zookeeper_session_connected(const char* path);
    void zookeeper_session_expired(const char *path);
    void zookeeper_session_event(int state, const char *path);
    void zookeeper_event(int type, int state, const char *path);

private:
    // zookeeper session连接成功事件
    virtual void on_zookeeper_session_connected(const char* path) {}

    // zookeeper session过期事件
    //
    // 可以调用reconnect_zookeeper()重连接zookeeper
    // 过期后可以选择调用reconnect_zookeeper()重连接zookeeper。
    // 但请注意，重连接成功后需要重新调用race_master()竞争master，
    // 简单点的做法是session过期后退出当前进程，通过重新启动的方式来竞争master
    //
    // 当session过期后（expired），要想继续使用则应当重连接，而且不能使用原来的ClientId重连接
    virtual void on_zookeeper_session_expired(const char *path) {}

    // session类zookeeper事件
    virtual void on_zookeeper_session_event(int state, const char *path) {}

    // 非session类zookeeper事件，包括但不限于：
    // 1) 节点被删除（state值为3，type值为2，即type值为ZOO_DELETED_EVENT）
    // 2) 节点的数据被修改（state值为3，type值为3，即type值为ZOO_CHANGED_EVENT）
    //
    // path 触发事件的path，如：/tmp/a
    virtual void on_zookeeper_event(int type, int state, const char *path) {}

private:
    bool _is_connected; // 是否已连接成功
    int _session_timeout_seconds; // zookeeper会话超时时长，单位为秒
    std::string _zk_nodes; // 连接zookeeper的节点字符串
    zhandle_t* _zk_handle; // zookeeper句柄
    const clientid_t* _zk_clientid;
};

inline static void zk_watcher(zhandle_t *zh, int type, int state, const char *path, void *context)
{
    CZookeeperHelper* self = static_cast<CZookeeperHelper*>(context);

    if (type != ZOO_SESSION_EVENT)
    {
        self->zookeeper_event(type, state, path);
    }
    else
    {
        // 1: ZOO_CONNECTING_STATE
        // 2: ZOO_ASSOCIATING_STATE
        // 3: ZOO_CONNECTED_STATE
        // -112: ZOO_EXPIRED_SESSION_STATE
        // 999: NOTCONNECTED_STATE_DEF

        if (ZOO_EXPIRED_SESSION_STATE == state)
        {
            // 需要重新调用zookeeper_init，简单点可以退出当前进程重启
            self->zookeeper_session_expired(path);
        }
        else if (ZOO_CONNECTED_STATE == state)
        {
            // zookeeper_init成功时type为ZOO_SESSION_EVENT，state为ZOO_CONNECTED_STATE
            self->zookeeper_session_connected(path);
        }
        else
        {
            self->zookeeper_session_event(state, path);
        }
    }

    //(void)zoo_set_watcher(zh, zk_watcher);
}

inline CZookeeperHelper::CZookeeperHelper() throw ()
    : _is_connected(false),
      _session_timeout_seconds(10),
      _zk_handle(NULL), _zk_clientid(NULL)
{
}

inline CZookeeperHelper::~CZookeeperHelper()
{
    (void)close_zookeeper();
}

inline int CZookeeperHelper::get_zk_data(const char* zk_path, std::string* zk_data, int data_size, bool keep_watch) const throw (utils::CException)
{
    const int watch = keep_watch? 1: 0;
    const int data_size_ = (data_size < 1)? 1: data_size;
    struct Stat stat;
    int datalen = data_size_;

    zk_data->resize(datalen);
    const int errcode = zoo_get(_zk_handle, zk_path, watch, const_cast<char*>(zk_data->data()), &datalen, &stat);
    if (ZOK == errcode)
    {
        zk_data->resize(datalen);
        return stat.dataLength;
    }
    else
    {
        THROW_EXCEPTION(utils::CStringUtils::format_string("%s (path://%s)", zerror(errcode), zk_path), errcode);
    }
}

inline std::string CZookeeperHelper::get_zk_data(const char* zk_path, int data_size, bool keep_watch) const throw (utils::CException)
{
    std::string zk_data;
    (void)get_zk_data(zk_path, &zk_data, data_size, keep_watch);
    return zk_data;
}

inline void CZookeeperHelper::connect_zookeeper(const std::string& zk_nodes, int session_timeout_seconds) throw (utils::CException)
{
    if (zk_nodes.empty())
    {
        THROW_EXCEPTION("parameter[zk_nodes] is empty", EINVAL);
    }
    else
    {
        _session_timeout_seconds = session_timeout_seconds*1000;
        _zk_nodes = zk_nodes;

        // 当连接不上时，会报如下错误（默认输出到stderr，可通过zoo_set_log_stream(FILE*)输出到文件）：
        // zk retcode=-4, errno=111(Connection refused): server refused to accept the client
        _zk_handle = zookeeper_init(_zk_nodes.c_str(), zk_watcher, _session_timeout_seconds, _zk_clientid, this, 0);
        if (NULL == _zk_handle)
        {
            THROW_EXCEPTION(zerror(errno), errno);
        }
    }
}

inline void CZookeeperHelper::close_zookeeper() throw (utils::CException)
{
    if (_zk_handle != NULL)
    {
        int errcode = zookeeper_close(_zk_handle);

        if (errcode != ZOK)
        {
            // 出错可能是因为内存不足或网络超时等
            THROW_EXCEPTION(zerror(errcode), errcode);
        }
        else
        {
            _is_connected = false;
            _zk_handle = NULL;
            _zk_clientid = NULL;
        }
    }
}

inline void CZookeeperHelper::reconnect_zookeeper() throw (utils::CException)
{
    close_zookeeper();

    _zk_handle = zookeeper_init(_zk_nodes.c_str(), zk_watcher, _session_timeout_seconds, _zk_clientid, this, 0);
    if (NULL == _zk_handle)
    {
        THROW_EXCEPTION(zerror(errno), errno);
    }
}

inline std::string CZookeeperHelper::get_connected_host() const throw ()
{
    std::string ip;
    uint16_t port = 0;
    get_connected_host(&ip, &port);
    return utils::CStringUtils::format_string("%s:%u", ip.c_str(), port);
}

inline bool CZookeeperHelper::get_connected_host(std::string* ip, uint16_t* port) const throw ()
{
    struct sockaddr_in6 addr_in6;
    socklen_t addr_len = sizeof(addr_in6);
    if (NULL == zookeeper_get_connected_host(_zk_handle, reinterpret_cast<struct sockaddr*>(&addr_in6), &addr_len))
    {
        return false;
    }

    const struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&addr_in6);
    if (AF_INET == addr->sa_family)
    {
        const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(addr);
        *ip = net::to_string(addr_in->sin_addr);
        *port = net::CUtils::net2host<uint16_t>(addr_in->sin_port);
    }
    else if (AF_INET6 == addr->sa_family)
    {
        *ip = net::CUtils::ipv6_tostring(addr_in6.sin6_addr.s6_addr32);
        *port = net::CUtils::net2host<uint16_t>(addr_in6.sin6_port);
    }
    else
    {
        return false;
    }

    return true;
}

inline bool CZookeeperHelper::is_connected() const throw ()
{
    if (!_is_connected)
    {
        return false;
    }
    else
    {
        // 3: ZOO_CONNECTED_STATE
        const int state = zoo_state(_zk_handle);
        return (ZOO_CONNECTED_STATE == state);
    }
}

inline int CZookeeperHelper::get_session_timeout_seconds() const
{
    return zoo_recv_timeout(_zk_handle);
}

inline bool CZookeeperHelper::race_master(const std::string& zk_path, const std::string& path_data, int* zk_errcode, std::string* zk_errmsg) throw ()
{
    // ZOO_EPHEMERAL|ZOO_SEQUENCE
    // 调用之前，需要确保_zk_path的父路径已存在
    // (-4)connection loss，比如为zookeeper_init指定了无效的hosts（一个有效的host也没有）
    const int errcode_ = zoo_create(_zk_handle, zk_path.c_str(), path_data.c_str(), static_cast<int>(path_data.size()), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);

    if (ZOK == errcode_)
    {
        if (zk_errcode != NULL)
            *zk_errcode = errcode_;
        if (zk_errmsg != NULL)
            *zk_errmsg = "SUCCESS";
        return true;
    }
    else
    {
        if (zk_errcode != NULL)
            *zk_errcode = errcode_;
        if (zk_errmsg != NULL)
            *zk_errmsg = zerror(errcode_);
        // ZOK operation completed successfully
        // ZNONODE the parent node does not exist
        // ZNODEEXISTS the node already exists
        return false;
    }
}

inline void CZookeeperHelper::create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data, int flags, const struct ACL_vector *acl) throw (utils::CException)
{
    const struct ACL_vector *acl_ = (NULL == acl)? &ZOO_OPEN_ACL_UNSAFE: acl;
    const std::string& zk_path = zk_parent_path + std::string("/") + zk_node_name;
    const int errcode = zoo_create(_zk_handle, zk_path.c_str(), zk_node_data.c_str(), static_cast<int>(zk_node_data.size()), acl_, flags, NULL, 0);
    if (errcode != ZOK)
    {
        THROW_EXCEPTION(zerror(errcode), errcode);
    }
}

inline void CZookeeperHelper::create_node(const std::string& zk_parent_path, const std::string& zk_node_name) throw (utils::CException)
{
    const std::string zk_node_data;
    const int flags = -1;
    const struct ACL_vector *acl = NULL;
    create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void CZookeeperHelper::create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data) throw (utils::CException)
{
    const int flags = -1;
    const struct ACL_vector *acl = NULL;
    create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void CZookeeperHelper::create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data, int flags) throw (utils::CException)
{
    const struct ACL_vector *acl = NULL;
    create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void CZookeeperHelper::create_node(const std::string& zk_parent_path, const std::string& zk_node_name, const std::string& zk_node_data, const struct ACL_vector *acl) throw (utils::CException)
{
    const int flags = -1;
    create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void CZookeeperHelper::delete_node(const std::string& zk_path, int version) throw (utils::CException)
{
    const int errcode = zoo_delete(_zk_handle, zk_path.c_str(), version);
    if (errcode != ZOK)
    {
        THROW_EXCEPTION(zerror(errcode), errcode);
    }
}

inline void CZookeeperHelper::zookeeper_session_connected(const char* path)
{
    _is_connected = true;
    _zk_clientid = zoo_client_id(_zk_handle);
    this->on_zookeeper_session_connected(path);
}

inline void CZookeeperHelper::zookeeper_session_expired(const char* path)
{
    _is_connected = false;
    this->on_zookeeper_session_expired(path);
}

inline void CZookeeperHelper::zookeeper_session_event(int state, const char *path)
{
    this->on_zookeeper_session_event(state, path);
}

inline void CZookeeperHelper::zookeeper_event(int type, int state, const char *path)
{
    this->on_zookeeper_event(type, state, path);
}

#endif // MOOON_HAVE_ZOOKEEPER == 1
NET_NAMESPACE_END
#endif // MOOON_NET_ZOOKEEPER_HELPER_H
