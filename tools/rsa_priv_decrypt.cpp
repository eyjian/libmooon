// Writed by yijian on 2024/01/10
#include <mooon/utils/rsa_helper.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    int sha_type;
    std::string sha;

    if (argc < 4)
    {
        fprintf(stderr, "usage: rsa_priv_decrypt RSAPaddingMode private_key_str data\n");
        return 1;
    }
    else
    {
        const uint8_t ras_padding_mode = mooon::utils::CStringUtils::string2int<uint8_t>(argv[1]);
        const std::string priv_key = argv[2]; // 私钥字符串
        const std::string encrypted_data = argv[3];
        std::string decrypted_data;

        try
        {
            mooon::utils::CRSAHelper::private_decrypt_bykey(priv_key, encrypted_data, &decrypted_data, (mooon::utils::RSAPaddingMode)ras_padding_mode);
        }
        catch (mooon::utils::CException& ex)
        {
            fprintf(stderr, "%s\n",ex.str().c_str());
        }
    }
}
