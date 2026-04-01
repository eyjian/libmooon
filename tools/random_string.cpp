// Writed by yijian on 2024/01/10
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: random_string length\n");
        return 1;
    }
    else
    {
        const uint8_t length = mooon::utils::CStringUtils::string2int<uint8_t>(argv[1]);
        fprintf(stdout, "%s\n", mooon::utils::CStringUtils::generate_random_string(length).c_str());
        return 0;
    }
}
