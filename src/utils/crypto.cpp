// Writed by yijian on 2018/12/23
#include "utils/crypto.h"
#include "utils/string_utils.h"
#if MOOON_HAVE_OPENSSL == 1
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
UTILS_NAMESPACE_BEGIN

// https://linux.die.net/man/3/bio
// https://linux.die.net/man/3/bio_push
// https://linux.die.net/man/3/bio_f_base64
// https://linux.die.net/man/3/bio_write
void base64_encode(const std::string& src, std::string* dest, bool no_newline)
{
    BUF_MEM* bptr;
    BIO* bmem = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());

    if (no_newline)
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, src.data(), static_cast<int>(src.length()));
    if (BIO_flush(b64));
    BIO_get_mem_ptr(b64, &bptr);

    dest->resize(bptr->length);
    memcpy(const_cast<char*>(dest->data()), bptr->data, bptr->length);
    BIO_free_all(b64);
}

// https://linux.die.net/man/3/bio_read
// https://www.openssl.org/docs/man1.0.2/crypto/BIO_read.html
void base64_decode(const std::string& src, std::string* dest, bool no_newline)
{
    BIO* bmem = BIO_new_mem_buf(const_cast<char*>(src.data()), static_cast<int>(src.length()));
    BIO* b64 = BIO_new(BIO_f_base64());

    if (no_newline)
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_push(b64, bmem);

    dest->resize(src.length());
    const int bytes = BIO_read(bmem, const_cast<char*>(dest->data()), static_cast<int>(src.length()));
    dest->resize(bytes);
    BIO_free_all(bmem);
}

void release_private_key(void** pkey)
{
    if (*pkey != nullptr)
    {
        EVP_PKEY_free((EVP_PKEY*)(*pkey));
        *pkey = nullptr;
    }
}

bool init_private_key_from_file(void** pkey, const std::string& private_key_file, std::string* errmsg)
{
    // 初始化为空
    *pkey = nullptr;

    // 读取私钥文件
    FILE* fp = fopen(private_key_file.c_str(), "r");
    if (fp == nullptr)
    {
        if (errmsg != nullptr)
            *errmsg = "unable to open private key file";
        return false;
    }
    else
    {
        EVP_PKEY* private_key = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
        fclose(fp);

        if (private_key == nullptr)
        {
            if (errmsg != nullptr)
                *errmsg = "unable to read private key from file";
            return false;
        }
        else
        {
            *pkey = private_key;
            return true;
        }
    }
}

bool init_private_key(void** pkey, const std::string& private_key_str, std::string* errmsg)
{
    // 初始化为空
    *pkey = nullptr;

    // 从字符串创建内存缓冲区
    BIO* bio = BIO_new_mem_buf(const_cast<char*>(private_key_str.c_str()), static_cast<int>(private_key_str.length()));
    if (!bio)
    {
        if (errmsg != nullptr)
            *errmsg = "unable to create BIO from private key string";
        return false;
    }
    else
    {
        // 从内存缓冲区读取私钥
        EVP_PKEY* private_key = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
        if (private_key == nullptr)
        {
            if (errmsg != nullptr)
                *errmsg = mooon::utils::CStringUtils::format_string("unable to read private key from string: %s", ERR_error_string(ERR_get_error(), NULL));
            return false;
        }
        else
        {
            // 释放内存缓冲区
            BIO_free(bio);
            *pkey = &private_key;
            return true;
        }
    }
}

bool RSA256_sign(std::string* signature_str, void* pkey, const std::string& data, std::string* errmsg)
{
    EVP_PKEY* pkey_ = (EVP_PKEY*)pkey;

    if (pkey_ == nullptr)
    {
        if (errmsg != nullptr)
            *errmsg = "private key is not initialized";
        return false;
    }
    else
    {
        // 获取 RSA 私钥
        RSA* rsa = EVP_PKEY_get1_RSA(pkey_);
        if (rsa == nullptr)
        {
            if (errmsg != nullptr)
                *errmsg = "unable to get RSA private key from EVP_PKEY";
            return false;
        }
        else
        {
            // 签名内容
            unsigned char signature_bytes[EVP_PKEY_size(pkey_)];
            size_t signature_len;
            EVP_MD_CTX* ctx = EVP_MD_CTX_create();
            EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, pkey_);
            EVP_DigestSignUpdate(ctx, data.c_str(), data.length());
            if (EVP_DigestSignFinal(ctx, signature_bytes, &signature_len) != 1)
            {
                if (errmsg != nullptr)
                    *errmsg = mooon::utils::CStringUtils::format_string("unable to sign data: %s", ERR_error_string(ERR_get_error(), NULL));
                return false;
            }
            else
            {
                BIO* bio = BIO_new(BIO_s_mem());
                BUF_MEM* buf_mem = nullptr;

                // 释放资源
                EVP_MD_CTX_destroy(ctx);
                RSA_free(rsa);

                // 将签名结果转换为 Base64 编码
                BIO_write(bio, signature_bytes, static_cast<int>(signature_len));
                BIO_get_mem_ptr(bio, &buf_mem);

                const std::string str(buf_mem->data, buf_mem->length);
                mooon::utils::base64_encode(str, signature_str);
                BIO_free(bio);

                return true;
            }
        }
    }
}

UTILS_NAMESPACE_END
#endif // MOOON_HAVE_OPENSSL
