// Writed by yijian on 2018/10/19
#include <mooon/sys/utils.h>
#include <mooon/utils/string_utils.h>

// argv[1] process name
// argv[2] signo
// argv[3] regex
int main(int argc, char* argv[])
{
    if ((argc != 3) && (argc != 4))
    {
        fprintf(stderr, "Usage: %s pname signo [0|1]\n", mooon::sys::CUtils::get_program_short_name().c_str());
        exit(1);
    }

    const char* process_name = argv[1];
    const char* signo_str = argv[2];
    int signo = 0;
    bool regex = false;

    // 第2个参数
    if (!mooon::utils::CStringUtils::string2int(signo_str, signo))
    {
        fprintf(stderr, "Invalid signo: %s\n", signo_str);
        fprintf(stderr, "Usage: %s pname signo [0|1]\n", mooon::sys::CUtils::get_program_short_name().c_str());
        exit(1);
    }

    if (4 == argc)
    {
        fprintf(stderr, "Usage: %s pname signo [0|1]\n", mooon::sys::CUtils::get_program_short_name().c_str());
        exit(1);
    }

    // 第3个参数
    if (0 == strcmp(argv[3], "0"))
        regex = false;
    else if (0 == strcmp(argv[3], "1"))
        regex = true;
    else
    {
        fprintf(stderr, "Usage: %s pname signo [0|1]\n", mooon::sys::CUtils::get_program_short_name().c_str());
        exit(1);
    }

    const std::pair<int, int> ret = mooon::sys::CUtils::killall(process_name, signo, regex);
    fprintf(stdout, "SUCCESS: %d, FAILURE: %d\n", ret.first, ret.second);
    return 0;
}
