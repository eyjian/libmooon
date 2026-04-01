// Writed by yijian on 2018/11/09
// 将一个zookeeper节点的数据写入到本地文件
#include <mooon/net/zookeeper_helper.h>
#include <mooon/utils/args_parser.h>

// 指定配置文件参数
STRING_ARG_DEFINE(conf, "", "The local configuration file path, example: --conf=/tmp/dist.conf");

// 指定存储配置文件的zookeeper
STRING_ARG_DEFINE(zookeeper, "", "The zookeeper nodes, example: --zookeeper=127.0.0.1:2181,127.0.0.2:2181");

// 指定存储配置的zookeeper节点（需有写权限）
STRING_ARG_DEFINE(zkpath, "", "The path of zookeeper, example: --zkpath=/tmp/a");

int main(int argc, char* argv[])
{
    std::string confdata;
    std::string errmsg;

    // 解析命令行参数
    if (!mooon::utils::parse_arguments(argc, argv, &errmsg))
    {
        if (!errmsg.empty())
            fprintf(stderr, "%s\n", errmsg.c_str());
        fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        exit(0);
    }

    // --conf
    if (mooon::argument::conf->value().empty())
    {
        fprintf(stderr, "Parameter[--conf] is not set\n");
        fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        exit(0);
    }

    // --zookeeper
    if (mooon::argument::zookeeper->value().empty())
    {
        fprintf(stderr, "Parameter[--zookeeper] is not set\n");
        fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        exit(0);
    }

    // --zkpath
    if (mooon::argument::zkpath->value().empty())
    {
        fprintf(stderr, "Parameter[--zkpath] is not set\n");
        fprintf(stderr, "%s\n", mooon::utils::g_help_string.c_str());
        exit(0);
    }

    try
    {
        mooon::net::CZookeeperHelper zookeeper;
        zookeeper.create_session(mooon::argument::zookeeper->value());
        const int n = zookeeper.store_data(mooon::argument::conf->value(), mooon::argument::zkpath->value(), false);
        fprintf(stdout, "bytes: %d\n", n);
        return 0;
    }
    catch (mooon::sys::CSyscallException& ex)
    {
        fprintf(stderr, "%s\n", ex.str().c_str());
        exit(1);
    }
    catch (mooon::utils::CException& ex)
    {
        fprintf(stderr, "%s\n", ex.str().c_str());
        exit(1);
    }
}
