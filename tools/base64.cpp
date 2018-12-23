// Writed by yijian on 2018/12/23
#include <mooon/utils/crypto.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    int action;
    std::string base64;

    if (argc < 2)
    {
        fprintf(stderr, "usage: base64 action(1: encode, 2: deconde) string\n");
        exit(1);
    }
    if (!mooon::utils::CStringUtils::string2int(argv[1], action) ||
        (action!=1 && action!=2))
    {
        fprintf(stderr, "usage: base64 action(1: encode, 2: deconde) string\n");
        exit(1);
    }
    else
    {
        const std::string& src = argv[2];
        std::string dest;
        if (1 == action)
            mooon::utils::base64_encode(src, &dest);
        else
            mooon::utils::base64_decode(src, &dest);
        printf("%s\n", dest.c_str());
        return 0;
    }
}
