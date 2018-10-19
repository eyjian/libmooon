// Writed by yijian on 2018/10/19
// get all pid by process name
#include <mooon/sys/utils.h>

// argv[1] process name
// argv[2] regex
int main(int argc, char* argv[])
{
    if ((argc != 2) && (argc != 3))
    {
        fprintf(stderr, "Usage: %s pname [0|1]\n", mooon::sys::CUtils::get_program_short_name().c_str());
        exit(1);
    }

    const char* process_name = argv[1];
    bool regex = false;

    if (3 == argc)
    {
        // 第2个参数
        if (0 == strcmp(argv[2], "0"))
            regex = false;
        else if (0 == strcmp(argv[2], "1"))
            regex = true;
        else
        {
            fprintf(stderr, "Usage: %s pname signo [0|1]\n", mooon::sys::CUtils::get_program_short_name().c_str());
            exit(1);
        }
    }

    std::vector<int64_t> pid_array;
    const int n = mooon::sys::CUtils::get_all_pid(process_name, &pid_array, regex);
    for (int i=0; i<n; ++i)
        fprintf(stdout, "%" PRId64"\n", pid_array[i]);
    return 0;
}
