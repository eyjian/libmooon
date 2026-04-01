// Writed by yijian on 2018/12/23
#include <mooon/utils/sha_helper.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    int sha_type;
    std::string sha;

    if (argc < 3)
    {
        fprintf(stderr, "usage: sha type(1/224/256/384/512) string\n");
        exit(1);
    }
    if (!mooon::utils::CStringUtils::string2int(argv[1], sha_type))
    {
        fprintf(stderr, "usage: sha type(1/224/256/384/512) string\n");
        exit(1);
    }
    else
    {
        const mooon::utils::SHAType sha_type_ = mooon::utils::int2shatype(sha_type);

        if (mooon::utils::SHA0 == sha_type_)
        {
            fprintf(stderr, "usage: sha type(1/224/256/384/512) string\n");
            exit(1);
        }
        else
        {
            sha = mooon::utils::CSHAHelper::lowercase_sha(sha_type_, "%s", argv[2]);
            printf("%s\n", sha.c_str());

            sha = mooon::utils::CSHAHelper::uppercase_sha(sha_type_, "%s", argv[2]);
            printf("%s\n", sha.c_str());
            return 0;
        }
    }
}
