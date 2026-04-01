/**
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
 *
 * Author: JianYi, eyjian@qq.com or eyjian@gmail.com
 */
#ifndef MOOON_SYS_MYSQL_DB_H
#define MOOON_SYS_MYSQL_DB_H
#include "mooon/sys/simple_db.h"
#include <stdarg.h>

#if MOOON_HAVE_MYSQL==1
SYS_NAMESPACE_BEGIN

// (1317)Query execution was interrupted
// (1040)Too many connections MySQL连接数过多
// (144)is marked as crashed and last (automatic?) repair failed
// (145)is marked as crashed and should be repaired 表crash了
// (29)not found (Errcode: 30 - Read-only file system)
// (126)Incorrect key file for table

// 如果以工厂方式：
// mooon::sys::DBConnection* db = (mooon::sys::DBConnection*)mooon::utils::CObjectFacotry::get_singleton()->create_object("mysql_connection");
// 使用CMySQLConnection，则编译时需要指定链接参数“-whole-archive”，
// 否则CMySQLConnection不会被链入，但这会产生副作用，需要链接libmooon.a所依赖的所有第三方库，
// 带“-whole-archive”的编译示例：
// g++ -g -o x x.cpp -I/usr/local/mooon/include -Wl,-whole-archive /usr/local/mooon/lib/libmooon.a -Wl,-no-whole-archive /usr/local/libssh2/lib/libssh2.a  -ldl -pthread /usr/local/sqlite3/lib/libsqlite3.a /usr/local/mysql/lib/libmysqlclient.a   /usr/local/curl/lib/libcurl.a /usr/local/libidn/lib/libidn.a /usr/local/c-ares/lib/libcares.a /usr/local/thrift/lib/libthrift.a /usr/local/openssl/lib/libssl.a /usr/local/openssl/lib/libcrypto.a
//
// 工厂方式示例：
// $ cat x.cpp
// #include <mooon/sys/simple_db.h>
// #include <mooon/utils/object.h>
//
// int main()
// {
//     mooon::sys::DBConnection* db = (mooon::sys::DBConnection*)mooon::utils::CObjectFacotry::get_singleton()->create_object("mysql_connection");
//     printf("%p\n", db); // 不指定“-whole-archive”时的db值为NULL
//     return 0;
// }

/**
 * MySQL版本的DB连接
 */
class CMySQLConnection: public CDBConnectionBase
{
public:
    // Call this function to initialize the MySQL library before you call any other MySQL function
    // 多线程环境必须调用library_init，并且必须在创建任何线程之前调用library_init
    static bool library_init(int argc=0, char **argv=NULL, char **groups=NULL);
    // library_end用于释放library_init分配的资源
    static void library_end();

    static bool is_duplicate(int errcode);

    // 双引号转成：\"
    // 单引号转成：\'
    // 单斜杠转成双斜杠
    // 注意不转义空格、|、?、<、>、{、}、:、~、@、!、(、)、`、#、%、,、;、&、-和_等
    static void escape_string(const std::string& str, std::string* escaped_str);

public:
    CMySQLConnection(size_t sql_max=8192, bool multistatements=false);
    ~CMySQLConnection();
    void* get_mysql_handle() const { return _mysql_handle; }

public:
    virtual bool is_syntax_exception(int errcode) const; // errcode值为1064
    virtual bool is_duplicate_exception(int errcode) const; // errcode值为1062
    virtual bool is_disconnected_exception(CDBException& db_error) const;
    virtual bool is_deadlock_exception(CDBException& db_error) const;
    virtual bool is_shutdowning_exception(CDBException& db_error) const;
    virtual bool is_notable_exception(CDBException& db_error) const; // 表不存在异常

    // 如果一条查询语句过慢，可能导致大量的lost错误消耗光MySQL连接
    // 因此遇到此错误时，需要考虑降慢重连接速度，即使开启了自动得连接！
    virtual bool is_lost_connection_exception(CDBException& db_error) const;
    virtual bool is_too_many_connections_exception(CDBException& db_error) const;

    virtual bool is_query_interrupted_exception(CDBException& db_error) const;
    virtual bool is_connect_host_exception(CDBException& db_error) const;
    virtual bool is_server_gone_exception(CDBException& db_error) const;
    virtual bool is_connection_error_exception(CDBException& db_error) const;
    virtual bool is_server_handshake_exception(CDBException& db_error) const;
    virtual bool is_ipsock_exception(CDBException& db_error) const;

    virtual std::string escape_string(const std::string& str) const;
    virtual void change_charset(const std::string& charset);
    virtual void open();
    virtual void close() throw ();
    virtual void reopen();

    // 如果update的值并没变化返回0，否则返回变修改的行数
    virtual uint64_t update(const char* format, ...) __attribute__((format(printf, 2, 3)));
    virtual uint64_t get_insert_id() const;
    virtual std::string str() throw ();

    virtual void ping();
    virtual void commit();
    virtual void rollback();

    /** 是否允许自动提交事务，注意只有open()或reopen()成功之后，才可以调用 */
    virtual void enable_autocommit(bool enabled);

public:
    // 如果构造函数的multistatements为true，即一次性执行多条语句（可为SELECT和UPDATE等组合），
    // 这时不能调用query和update来执行，而应当调用multi_statements执行。
    // 执行的结果通过调用fetch_results来取得，
    // fetch_results返回两个值，如果第一个值为true，表示返回的是SELECT结果，否则是UPDATE等结果。
    //
    // 使用示例（注意fetch_results和have_more_results的调用顺序）：
    // CMySQLConnection mysql
    // mysql.multi_statements(sql.c_str(), sql.size());
    // while (true) {
    //     DBTable dbtable;
    //     const std::pair<bool, uint64_t> ret = mysql.fetch_results(dbtable);
    //     if (ret.first) {
    //         handle(dbtable);
    //     }
    //     else {
    //         printf("affected rows: %" PRIu64"\n", ret.second);
    //     }
    //     if (!mysql.have_more_results())
    //         break;
    // }
    //
    // CR_COMMANDS_OUT_OF_SYNC Commands were executed in an improper order.
    void multi_statements(const char* sql, int sql_length);

    // 注意fetch_results和have_more_results的调用顺序
    std::pair<bool,uint64_t> fetch_results(DBTable& db_table);

    // 注意fetch_results和have_more_results的调用顺序
    bool have_more_results() const;

private:
    virtual void do_query(DBTable& db_table, const char* sql, int sql_length);

private:
    void do_open();

private:
    void* _mysql_handle; // MySQL句柄
    int _client_flag;
};

SYS_NAMESPACE_END
#endif // MOOON_HAVE_MYSQL
#endif // MOOON_SYS_MYSQL_DB_H
