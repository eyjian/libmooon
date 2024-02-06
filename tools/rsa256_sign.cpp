// Writed by yijian on 2024/01/09
#include <mooon/utils/rsa_helper.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    int sha_type;
    std::string sha;

    if (argc < 3)
    {
        fprintf(stderr, "usage: rsa256_sign private_filepath data_to_sign\n");
        return 1;
    }
    else
    {
        const std::string data = argv[2];
        const std::string& private_key_file = argv[1];
        mooon::utils::CRSAHelper rsa_helper(private_key_file);

        try
        {
            std::string signature;

            rsa_helper.init();
            mooon::utils::rsa256sign(&signature, rsa_helper.pkey(), data);
            fprintf(stdout, "signature: %s\n", signature.c_str());
            return 0;
        }
        catch (mooon::utils::CException& ex)
        {
            fprintf(stderr, "%s\n", ex.str().c_str());
            return 1;
        }
    }
}
