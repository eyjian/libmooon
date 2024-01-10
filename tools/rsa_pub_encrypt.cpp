// Writed by yijian on 2024/01/10
#include <mooon/utils/rsa_helper.h>
#include <mooon/utils/string_utils.h>

int main(int argc, char* argv[])
{
    int sha_type;
    std::string sha;

    if (argc < 4)
    {
        fprintf(stderr, "usage: rsa_pub_encrypt RSAPaddingMode public_key_str data\n");
        return 1;
    }
    else
    {
        const uint8_t ras_padding_mode = mooon::utils::CStringUtils::string2int<uint8_t>(argv[1]);
        const std::string pub_key = argv[2]; // 公钥字符串
        const std::string decrypted_data = argv[3];
        std::string encrypted_data;

        try
        {
            mooon::utils::CRSAHelper::public_encrypt_bykey(pub_key, decrypted_data, &encrypted_data, (mooon::utils::RSAPaddingMode)ras_padding_mode);
        }
        catch (mooon::utils::CException& ex)
        {
            fprintf(stderr, "%s\n",ex.str().c_str());
        }
    }
}
