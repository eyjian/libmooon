// Writed by yijian on 2024/01/10
#include <mooon/utils/exception.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: charset_conv src_charset dest_charset src_str\n");
        return 1;
    }
    else
    {
        const std::string src_charset = argv[1];
        const std::string dest_charset = argv[2];
        const std::string src_str = argv[3];
        std::string dest_str;

        try
        {
            mooon::utils::CStringUtils::charset_conv(&dest_str, src_str, dest_charset, src_charset);
            fprintf(stdout, "%s\n", dest_str.c_str());
        }
        catch (mooon::utils::CException& ex)
        {
            fprintf(stderr, "%s\n", ex.str().c_str());
        }
    }
}
