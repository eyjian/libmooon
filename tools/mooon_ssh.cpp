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
 * Author: eyjian@qq.com or eyjian@gmail.com
 */
// 编译方法：
// 编译libmooon即可生成mooon_upload
// 但libmooon依赖开源的libssh2库（http://www.libssh2.org/）
//
// 批量远程执行shell命令工具
// 使用示例（-p指定端口，-P指定密码）：
// mooon_ssh -u=root -P=test -p=2016 -h="127.0.0.1,192.168.0.1" -c='ls /tmp&&ps aux|grep -c test'
//
// 可通过为参数“-thr”指定大于1的值并发执行
//
// 可环境变量HOSTS替代参数“-h”
// 可环境变量USER替代参数“-u”
// 可环境变量PASSWORD替代参数“-p”
#include "mooon/net/libssh2.h" // 提供远程执行命令接口
#include "mooon/sys/error.h"
#include "mooon/sys/stop_watch.h"
#include "mooon/sys/thread_engine.h"
#include "mooon/utils/args_parser.h"
#include "mooon/utils/print_color.h"
#include "mooon/utils/string_utils.h"
#include "mooon/utils/tokener.h"
#include <iostream>
#include <sstream>

// 逗号分隔的远程主机IP列表
STRING_ARG_DEFINE(h, "", "Connect to the remote machines on the given hosts separated by comma, can be replaced by environment variable 'H', example: -h='192.168.1.10,192.168.1.11'");
// 远程主机的sshd端口号
INTEGER_ARG_DEFINE(uint16_t, P, 22, 10, 65535, "Specifies the port to connect to on the remote machines, can be replaced by environment variable 'PORT'");
// 用户名
STRING_ARG_DEFINE(u, "", "Specifies the user to log in as on the remote machines, can be replaced by environment variable 'U'");
// 密码
STRING_ARG_DEFINE(p, "", "The password to use when connecting to the remote machines, can be replaced by environment variable 'P'");
// 连接超时，单位为秒
INTEGER_ARG_DEFINE(uint16_t, t, 60, 1, 65535, "The number of seconds before connection timeout");

// 被执行的命令，可为一条或多条命令，如：ls /&&whoami
STRING_ARG_DEFINE(c, "", "The command is executed on the remote machines, example: -c='grep ERROR /tmp/*.log'");

// 输出级别
// 0 静默模式
// 1 最详细
// 2 只输出总结
INTEGER_ARG_DEFINE(uint8_t, v, 1, 0, 2, "Verbosity, how much troubleshooting info to print");

// 线程数（parallel），多线程并行执行
INTEGER_ARG_DEFINE(int, thr, 1, 0, 2018, "The number of threads to parallel execute, can be replaced by environment variable 'THR'");

// 结果信息
struct ResultInfo
{
    bool success; // 为true表示执行成功
    std::string ip; // 远程host的IP地址
    uint32_t seconds; // 运行花费的时长，精确到秒

    ResultInfo()
        : success(false), seconds(0)
    {
    }

    std::string str() const
    {
        std::string tag = success? "SUCCESS": "FAILURE";
        return mooon::utils::CStringUtils::format_string("[%s %s]: %u seconds", ip.c_str(), tag.c_str(), seconds);
    }
};

inline std::ostream& operator <<(std::ostream& out, const struct ResultInfo& result)
{
    std::string tag = result.success? "SUCCESS": "FAILURE";
    out << "[" PRINT_COLOR_YELLOW << result.ip << PRINT_COLOR_NONE" " << tag << "] " << result.seconds << " seconds";
    return out;
}

// 取得线程数
static int get_num_of_threads(int num_hosts);

// thread 是否运行在线程中
static void mooon_ssh(bool thread, struct ResultInfo& result, const std::string& remote_host_ip, int port, const std::string& user, const std::string& password, const std::string& commands);

struct SshTask
{
    struct ResultInfo* result;
    std::string remote_host_ip;
    int port;
    std::string user;
    std::string password;
    std::string commands;
};

class CSShThread
{
public:
    CSShThread(int index) { _index = index; }
    void add_task(struct ResultInfo& result, const std::string& remote_host_ip, int port, const std::string& user, const std::string& password, const std::string& commands);
    void run();

private:
    int _index;
    std::vector<struct SshTask> _tasks;
};

// 使用示例：
// mooon_ssh -u=root -P=test -p=2016 -h="127.0.0.1,192.168.0.1" -c='ls /tmp&&ps aux|grep -c test'
//
// 可环境变量H替代参数“-h”
// 可环境变量U替代参数“-u”
// 可环境变量P替代参数“-p”
int main(int argc, char* argv[])
{
#if MOOON_HAVE_LIBSSH2 == 1
    // 解析命令行参数
    std::string errmsg;
    if (!mooon::utils::parse_arguments(argc, argv, &errmsg))
    {
        if (errmsg.empty())
        {
            fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        }
        else
        {
            fprintf(stderr, "parameter error: %s\n\n", errmsg.c_str());
            fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        }
        exit(1);
    }

    uint16_t port = mooon::argument::P->value();
    std::string commands = mooon::argument::c->value();
    std::string hosts = mooon::argument::h->value();
    std::string user = mooon::argument::u->value();
    std::string password = mooon::argument::p->value();
    mooon::utils::CStringUtils::trim(commands);
    mooon::utils::CStringUtils::trim(hosts);
    mooon::utils::CStringUtils::trim(user);
    mooon::utils::CStringUtils::trim(password);

    // 检查参数（-P）
    const char* port_ = getenv("PORT");
    if (port_ != NULL)
    {
        // 优先使用环境变量的值，但如果不是合法的值，则仍然使用参数值
        (void)mooon::utils::CStringUtils::string2int(port_, port);
    }

    // 检查参数（-c）
    if (commands.empty())
    {
        fprintf(stderr, "parameter[-c]'s value not set\n\n");
        fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
        exit(1);
    }

    // 检查参数（-h）
    if (hosts.empty())
    {
        // 尝试从环境变量取值
        const char* hosts_ = getenv("H");
        if (NULL == hosts_)
        {
            fprintf(stderr, "parameter[-h] or environment `H` not set\n\n");
            fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
            exit(1);
        }

        hosts= hosts_;
        mooon::utils::CStringUtils::trim(hosts);
        if (hosts.empty())
        {
            fprintf(stderr, "parameter[-h] or environment `H` not set\n");
            fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
            exit(1);
        }
    }

    // 检查参数（-u）
    if (user.empty())
    {
        // 尝试从环境变量取值
        const char* user_ = getenv("U");
        if (NULL == user_)
        {
            fprintf(stderr, "parameter[-u] or environment `U` not set\n\n");
            fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
            exit(1);
        }

        user= user_;
        mooon::utils::CStringUtils::trim(user);
        if (user.empty())
        {
            fprintf(stderr, "parameter[-u] or environment `U` not set\n\n");
            fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
            exit(1);
        }
    }

    // 检查参数（-p）
    if (password.empty())
    {
        // 尝试从环境变量取值
        const char* password_ = getenv("P");
        if (NULL == password_)
        {
            fprintf(stderr, "parameter[-p] or environment `P` not set\n\n");
            fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
            exit(1);
        }

        password= password_;
        if (password.empty())
        {
            fprintf(stderr, "parameter[-p] or environment `P` not set\n\n");
            fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
            exit(1);
        }
    }

    time_t start_seconds = time(NULL);
    std::vector<std::string> hosts_ip;
    const std::string& remote_hosts_ip = hosts;
    const int num_remote_hosts_ip = mooon::utils::CTokener::split(&hosts_ip, remote_hosts_ip, ",", true);
    if (0 == num_remote_hosts_ip)
    {
        fprintf(stderr, "parameter[-h] or environment `H` error\n\n");
        fprintf(stderr, "%s\n", mooon::utils::CArgumentContainer::get_singleton()->usage_string().c_str());
        exit(1);
    }

    // 先创建线程对象，但不要启动线程
    const int num_threads = get_num_of_threads((int)hosts_ip.size());
    std::vector<CSShThread*> ssh_threads;
    if (num_threads > 1)
    {
        for (int i=0; i<num_threads; ++i)
        {
            CSShThread* ssh_thread = new CSShThread(i);
            ssh_threads.push_back(ssh_thread);
        }
    }

    mooon::net::CLibssh2::init();
    std::vector<struct ResultInfo> results(num_remote_hosts_ip);
    for (int i=0; i<num_remote_hosts_ip; ++i)
    {
        struct ResultInfo& result = results[i];
        const std::string& remote_host_ip = hosts_ip[i];

        if (num_threads <= 1)
        {
            mooon_ssh(false, result, remote_host_ip, port, user, password, commands);
        }
        else
        {
            CSShThread* ssh_thread = ssh_threads[i % num_threads];
            ssh_thread->add_task(result, remote_host_ip, port, user, password, commands);
        }
    } // for

    // 启动所有线程
    if (num_threads > 1)
    {
        std::vector<mooon::sys::CThreadEngine*> thread_engines(num_threads);
        for (int i=0; i<num_threads; ++i)
        {
            CSShThread* ssh_thread = ssh_threads[i];
            thread_engines[i] = new mooon::sys::CThreadEngine(mooon::sys::bind(&CSShThread::run, ssh_thread));
        }

        for (int i=0; i<num_threads; ++i)
        {
            thread_engines[i]->join();
            delete ssh_threads[i];
            delete thread_engines[i];
        }

        thread_engines.clear();
        ssh_threads.clear();
    }
    mooon::net::CLibssh2::fini();

    std::cout << std::endl;
    if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 1))
    {
        // 输出总结
        std::cout << "================================" << std::endl;
    }

    int num_success = 0; // 成功的个数
    int num_failure = 0; // 失败的个数
    for (std::vector<struct ResultInfo>::size_type i=0; i<results.size(); ++i)
    {
        const struct ResultInfo& result_info = results[i];

        if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 2))
        {
            std::cout << result_info << std::endl;
        }

        if (result_info.success)
            ++num_success;
        else
            ++num_failure;
    }

    const time_t end_seconds = time(NULL);
    int cost_seconds = 0;
    if (end_seconds > start_seconds)
    {
        cost_seconds = static_cast<int>(end_seconds - start_seconds);
    }
    if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 2))
    {
        std::cout << "SUCCESS: " << num_success << ", FAILURE: " << num_failure << ", SECONDS: "<< cost_seconds<< std::endl;
    }
#else
    fprintf(stderr, "NOT IMPLEMENT! please install libssh2 (https://www.libssh2.org/) into /usr/local/libssh2 and recompile.\n");
#endif // MOOON_HAVE_LIBSSH2 == 1

    return (0 == num_failure)? 0: 1;
}

int get_num_of_threads(int num_hosts)
{
    int num_threads = mooon::argument::thr->value();

    const char* str = getenv("THR");
    if (str != NULL)
    {
        if (!mooon::utils::CStringUtils::string2int(str, num_threads))
            num_threads = mooon::argument::thr->value();
    }

    if ((num_threads > num_hosts) || (0 == num_threads))
        num_threads = num_hosts;
    return num_threads;
}

void mooon_ssh(bool thread, struct ResultInfo& result, const std::string& remote_host_ip, int port, const std::string& user, const std::string& password, const std::string& commands)
{
    bool color = true;
    int num_bytes = 0;
    int exitcode = 0;
    std::string exitsignal;
    std::string errmsg;
    std::string screen, str;

    result.ip = remote_host_ip;
    if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 1))
    {
        str = mooon::utils::CStringUtils::format_string("[" PRINT_COLOR_YELLOW"%s" PRINT_COLOR_NONE"]\n", remote_host_ip.c_str());
        str += PRINT_COLOR_GREEN;
        screen += str;
        if (!thread)
            fprintf(stdout, "%s", str.c_str());
    }

    mooon::sys::CStopWatch stop_watch;
    try
    {
        std::stringstream out;
        mooon::net::CLibssh2 libssh2(remote_host_ip, port, user, password, mooon::argument::t->value());

        libssh2.remotely_execute(commands, out, &exitcode, &exitsignal, &errmsg, &num_bytes);
        str = PRINT_COLOR_NONE;
        screen += str;
        if (!thread)
            fprintf(stdout, "%s", str.c_str());
        color = false; // color = true;

        result.seconds = stop_watch.get_elapsed_microseconds() / 1000000;
        if ((0 == exitcode) && exitsignal.empty())
        {
            result.success = true;

            if (!thread)
                fprintf(stdout, "%s", out.str().c_str());
            else
                screen += out.str();
            if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 1))
            {
                str = mooon::utils::CStringUtils::format_string("[" PRINT_COLOR_YELLOW"%s" PRINT_COLOR_NONE"] SUCCESS (%u seconds)\n", remote_host_ip.c_str(), result.seconds);
                screen += str;
                if (!thread)
                    fprintf(stdout, "%s", str.c_str());
            }
        }
        else
        {
            result.success = false;

            if (!thread)
                fprintf(stderr, "%s", out.str().c_str());
            else
                screen += out.str();
            if (exitcode != 0)
            {
                if (1 == exitcode)
                {
                    str = mooon::utils::CStringUtils::format_string("command return %d\n", exitcode);
                    screen += str;
                    if (!thread)
                        fprintf(stderr, "%s", str.c_str());
                }
                else if (126 == exitcode)
                {
                    str = mooon::utils::CStringUtils::format_string("%d: command not executable\n", exitcode);
                    screen += str;
                    if (!thread)
                        fprintf(stderr, "%s", str.c_str());
                }
                else if (127 == exitcode)
                {
                    str = mooon::utils::CStringUtils::format_string("%d: command not found\n", exitcode);
                    screen += str;
                    if (!thread)
                        fprintf(stderr, "%s", str.c_str());
                }
                else if (255 == exitcode)
                {
                    str = mooon::utils::CStringUtils::format_string("%d: command not found\n", 127);
                    screen += str;
                    if (!thread)
                        fprintf(stderr, "%s", str.c_str());
                }
                else
                {
                    str = mooon::utils::CStringUtils::format_string("%d: %s\n", exitcode, mooon::sys::Error::to_string(exitcode).c_str());
                    screen += str;
                    if (!thread)
                        fprintf(stderr, "%s", str.c_str());
                }
            }
            else if (!exitsignal.empty())
            {
                str = mooon::utils::CStringUtils::format_string("%s: %s\n", exitsignal.c_str(), errmsg.c_str());
                screen += str;
                if (!thread)
                    fprintf(stderr, "%s", str.c_str());
            }
        }
    }
    catch (mooon::sys::CSyscallException& ex)
    {
        result.seconds = stop_watch.get_elapsed_microseconds() / 1000000;

        if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 1))
        {
            if (color)
            {
                screen += PRINT_COLOR_NONE; // color = true;
                screen += str;
                if (!thread)
                    fprintf(stdout, "%s", str.c_str());
            }

            str = mooon::utils::CStringUtils::format_string("[" PRINT_COLOR_RED"%s" PRINT_COLOR_NONE"] failed: %s\n", remote_host_ip.c_str(), ex.str().c_str());
            screen += str;
            if (!thread)
                fprintf(stderr, "%s", str.c_str());
        }
    }
    catch (mooon::utils::CException& ex)
    {
        result.seconds = stop_watch.get_elapsed_microseconds() / 1000000;

        if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 1))
        {
            if (color)
            {
                str = PRINT_COLOR_NONE; // color = true;
                screen += str;
                if (!thread)
                    fprintf(stdout, "%s", str.c_str());
            }

            str = mooon::utils::CStringUtils::format_string("[" PRINT_COLOR_RED"%s" PRINT_COLOR_NONE"] failed: %s\n", remote_host_ip.c_str(), ex.str().c_str());
            screen += str;
            if (!thread)
                fprintf(stderr, "%s", str.c_str());
        }
    }

    if ((mooon::argument::v->value() >= 1) && (mooon::argument::v->value() <= 1))
    {
        str = "\n";
        screen += str;
        if (!thread)
            fprintf(stdout, "%s", str.c_str());
    }
    if (thread)
    {
        write(STDIN_FILENO, screen.c_str(), screen.size());
    }
}

void CSShThread::run()
{
    for (std::vector<struct SshTask>::size_type i=0; i<_tasks.size(); ++i)
    {
        const struct SshTask& task = _tasks[i];
        mooon_ssh(true, *task.result, task.remote_host_ip, task.port, task.user, task.password, task.commands);
    }
}

void CSShThread::add_task(struct ResultInfo& result, const std::string& remote_host_ip, int port, const std::string& user, const std::string& password, const std::string& commands)
{
    struct SshTask task;
    task.result = &result;
    task.remote_host_ip = remote_host_ip;
    task.port = port;
    task.user = user;
    task.password = password;
    task.commands = commands;
    _tasks.push_back(task);
}
