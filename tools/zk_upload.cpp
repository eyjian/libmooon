// Writed by yijian on 2018/11/09
// 将一个文件作写入到zookeeper的指定节点
#include <mooon/net/zookeeper_helper.h>
#include <mooon/utils/args_parser.h>

// 指定配置文件参数
STRING_ARG_DEFINE(conf, "", "The local configuration file path, example: --conf=/tmp/dist.conf");

// 指定存储配置文件的zookeeper
STRING_ARG_DEFINE(zookeeper, "", "The zookeeper nodes, example: --zookeeper=127.0.0.1:2181,127.0.0.2:2181");

// 指定存储配置的zookeeper节点（需有写权限）
STRING_ARG_DEFINE(zkpath, "", "The path of zookeeper, example: --zkpath=/tmp/a");

// 从配置文件读取配置
static void read_conf(std::string* confdata);

// 上传配置到zookeeper
static int upload_conf(const std::string& confdata);

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

    read_conf(&confdata);
    return upload_conf(confdata);
}

void read_conf(std::string* confdata)
{
    int errcode = 0;
    struct stat st;

    const int fd = open(mooon::argument::conf->c_value(), O_RDONLY);
    if (-1 == fd)
    {
        errcode = errno;
        THROW_SYSCALL_EXCEPTION(mooon::utils::CStringUtils::format_string("open %s failed: %s", mooon::argument::conf->c_value(), strerror(errcode)), errcode, "open");
    }

    if (-1 == fstat(fd, &st))
    {
        close(fd);
        THROW_SYSCALL_EXCEPTION(mooon::utils::CStringUtils::format_string("stat %s failed: %s", mooon::argument::conf->c_value(), strerror(errcode)), errcode, "fstat");
    }
    else
    {
        confdata->resize(st.st_size);

        const int n = read(fd, const_cast<char*>(confdata->data()), st.st_size);
        if (n == static_cast<int>(st.st_size))
        {
            close(fd);
        }
        else
        {
            close(fd);
            THROW_SYSCALL_EXCEPTION(mooon::utils::CStringUtils::format_string("read %s failed: %s", mooon::argument::conf->c_value(), strerror(errcode)), errcode, "read");
        }
    }
}

int upload_conf(const std::string& confdata)
{
    try
    {
        mooon::net::CZookeeperHelper zookeeper;
        zookeeper.create_session(mooon::argument::zookeeper->value());
        zookeeper.set_zk_data(mooon::argument::zkpath->value(), confdata);
        fprintf(stdout, "Publish conf to zookeeper ok\n");
        return 0;
    }
    catch (mooon::utils::CException& ex)
    {
        fprintf(stderr, "%s\n", ex.str().c_str());
        return 1;
    }
}
