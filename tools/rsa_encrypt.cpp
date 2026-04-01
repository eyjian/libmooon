// Writed by yijian on 2024/02/06
#include <mooon/utils/crypto.h>
#include <mooon/utils/rsa_helper.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    std::string sha;

    if (argc < 3)
    {
        fprintf(stderr, "usage: rsa_encrypt public_filepath data_to_encrypt\n");
        return 1;
    }
    else
    {
        const std::string plaintext_data = argv[2];
        const std::string& public_key_filepath = argv[1];
        mooon::utils::CRsaPublicHelper rsa_helper(public_key_filepath);

        try
        {
            std::string base64_encrypted_data;
            std::string encrypted_data;

            rsa_helper.init();
            mooon::utils::rsa_encrypt(&encrypted_data, plaintext_data, rsa_helper.public_key());
            mooon::utils::base64_encode(encrypted_data, &base64_encrypted_data);
            fprintf(stdout, "ciphertext: %s\n", base64_encrypted_data.c_str());
            return 0;
        }
        catch (mooon::utils::CException& ex)
        {
            fprintf(stderr, "%s\n", ex.str().c_str());
            return 1;
        }
    }
}
