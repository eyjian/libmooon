// Writed by yijian on 2024/02/06
#include <mooon/utils/crypto.h>
#include <mooon/utils/rsa_helper.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    int sha_type;
    std::string sha;

    if (argc < 3)
    {
        fprintf(stderr, "usage: rsa_decrypt private_filepath base64_data_to_decrypt\n");
        return 1;
    }
    else
    {
        const std::string base64_data = argv[2];
        const std::string& private_key_filepath = argv[1];
        mooon::utils::CRsaPrivateHelper rsa_helper(private_key_filepath);

        try
        {
            std::string encrypted_data;
            std::string decrypted_data;

            rsa_helper.init();
            mooon::utils::base64_decode(base64_data, &encrypted_data);
            mooon::utils::rsa_decrypt(&decrypted_data, encrypted_data, rsa_helper.pkey(), rsa_helper.pkey_ctx());
            fprintf(stdout, "signature: %s\n", decrypted_data.c_str());
            return 0;
        }
        catch (mooon::utils::CException& ex)
        {
            fprintf(stderr, "%s\n", ex.str().c_str());
            return 1;
        }
    }
}
