// Writed by yijian on 2024/01/09
#include <mooon/utils/crypto.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    int sha_type;
    std::string sha;

    if (argc < 3)
    {
        fprintf(stderr, "usage: rsa256_sign private_file data\n");
        return 1;
    }
    else
    {
        const std::string& private_key_file = argv[1];
        std::string errmsg;
        void* pkey = nullptr;

        if (!mooon::utils::init_private_key_from_file(&pkey, private_key_file, &errmsg))
        {
            fprintf(stderr, "init_private_key_from_file error: %s\n", errmsg.c_str());
            return 1;
        }
        else
        {
            const std::string data = argv[2];
            std::string signature;

            if (!mooon::utils::RSA256_sign(&signature, pkey, data, &errmsg))
            {
                fprintf(stderr, "RSA256_sign error: %s\n", errmsg.c_str());
                mooon::utils::release_private_key(&pkey);
                return 1;
            }
            else
            {
                fprintf(stdout, "signature: %s\n", signature.c_str());
                mooon::utils::release_private_key(&pkey);
                return 0;
            }
        }
    }
}
