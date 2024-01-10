/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: jian yi, eyjian@qq.com or eyjian@gmail.com
 */
#include "mooon/utils/rsa_helper.h"
#include "mooon/utils/crypto.h"
#include "mooon/utils/scoped_ptr.h"
#if MOOON_HAVE_OPENSSL == 1
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
UTILS_NAMESPACE_BEGIN

// PKEY 为 Private Key 的缩写，可以用来表示多种类型的公钥和私钥，如 RSA、DSA、EC 等
// EVP 为 Envelope 的缩写，指封装了多种密码算法的接口，可以通过这个接口使用各种不同的密码算法

// 生成密钥文件（包含公钥和私钥两部分）：
// openssl genrsa -out abc.key 1024
//
// 提取公钥：
// openssl rsa -in abc.key -pubout -out pub.key
//
// 用公钥加密文件：
// openssl rsautl -encrypt -in test.txt -inkey pub.key -pubin -out test.dat
//
// 用私钥解密文件：
// openssl rsautl -decrypt -in test.dat -inkey abc.key -out test.txt

// 参考：
// http://hayageek.com/rsa-encryption-decryption-openssl-c/
void CRSAHelper::public_encrypt_bykeyfile(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(pub_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open public key file://%s failed: %s", pub_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read public key from file://%s failed: %s", pub_keyfile.c_str(), buf.get()), errcode);
    }

    try
    {
        public_encrypt(rsa, instr, outstr, mode);
        RSA_free(rsa);
        fclose(fp);
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (fp != NULL)
            fclose(fp);
        throw;
    }
}

void CRSAHelper::private_decrypt_bykeyfile(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(priv_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open private key file://%s failed: %s", priv_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read private key from file://%s failed: %s", priv_keyfile.c_str(), buf.get()), errcode);
    }

    try
    {
        private_decrypt(rsa, instr, outstr, mode);
        RSA_free(rsa);
        fclose(fp);
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (fp != NULL)
            fclose(fp);
        throw;
    }
}

void CRSAHelper::private_encrypt_bykeyfile(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(priv_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open private key file://%s failed: %s", priv_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read private key from file://%s failed: %s", priv_keyfile.c_str(), buf.get()), errcode);
    }

    try
    {
        private_encrypt(rsa, instr, outstr, mode);
        RSA_free(rsa);
        fclose(fp);
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (fp != NULL)
            fclose(fp);
        throw;
    }
}

void CRSAHelper::public_decrypt_bykeyfile(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(pub_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open public key from file://%s failed: %s", pub_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read public key from file://%s failed: %s", pub_keyfile.c_str(), buf.get()), errcode);
    }

    try
    {
        public_decrypt(rsa, instr, outstr, mode);
        RSA_free(rsa);
        fclose(fp);
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (fp != NULL)
            fclose(fp);
        throw;
    }
}

void CRSAHelper::public_encrypt_bykey(const std::string& pub_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    BIO* keybio = NULL;
    RSA* rsa = NULL;

    // BIO_s_mem() return the memory BIO method function.
    // A memory BIO is a source/sink BIO which uses memory for its I/O.
    // Data written to a memory BIO is stored in a BUF_MEM structure which is
    // extended as appropriate to accommodate the stored data.
    //
    // https://linux.die.net/man/3/bio_new_mem_buf
    keybio = BIO_new_mem_buf((void*)pub_key.c_str(), -1);
    if (keybio == NULL)
    {
        errcode = ERR_get_error();
        THROW_EXCEPTION("failed to create public key BIO", errcode);
    }

    try
    {
        PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
        if (rsa == NULL)
        {
            errcode = ERR_get_error();
            THROW_EXCEPTION("failed to read public key", errcode);
        }
        else
        {
            public_encrypt(rsa, instr, outstr, mode);
            RSA_free(rsa);
            BIO_free(keybio);
        }
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (keybio != NULL)
            BIO_free(keybio);
        throw;
    }
}

void CRSAHelper::private_decrypt_bykey(const std::string& priv_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    BIO* keybio = NULL;
    RSA* rsa = NULL;

    keybio = BIO_new_mem_buf((void*)priv_key.c_str(), -1);
    if (keybio == NULL)
    {
        errcode = ERR_get_error();
        THROW_EXCEPTION("failed to create private key BIO", errcode);
    }

    try
    {
        PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
        if (rsa == NULL)
        {
            errcode = ERR_get_error();
            THROW_EXCEPTION("failed to read private key", errcode);
        }
        else
        {
            private_decrypt(rsa, instr, outstr, mode);
            RSA_free(rsa);
            BIO_free(keybio);
        }
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (keybio != NULL)
            BIO_free(keybio);
        throw;
    }
}

void CRSAHelper::private_encrypt_bykey(const std::string& priv_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    BIO* keybio = NULL;
    RSA* rsa = NULL;

    keybio = BIO_new_mem_buf((void*)priv_key.c_str(), -1);
    if (keybio == NULL)
    {
        errcode = ERR_get_error();
        THROW_EXCEPTION("failed to create private key BIO", errcode);
    }

    try
    {
        PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
        if (rsa == NULL)
        {
            errcode = ERR_get_error();
            THROW_EXCEPTION("failed to read public key", errcode);
        }
        else
        {
            private_encrypt(rsa, instr, outstr, mode);
            RSA_free(rsa);
            BIO_free(keybio);
        }
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (keybio != NULL)
            BIO_free(keybio);
        throw;
    }
}

void CRSAHelper::public_decrypt_bykey(const std::string& pub_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    BIO* keybio = NULL;
    RSA* rsa = NULL;

    keybio = BIO_new_mem_buf((void*)pub_key.c_str(), -1);
    if (keybio == NULL)
    {
        errcode = ERR_get_error();
        THROW_EXCEPTION("failed to create public key BIO", errcode);
    }

    try
    {
        PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
        if (rsa == NULL)
        {
            errcode = ERR_get_error();
            THROW_EXCEPTION("failed to read public key", errcode);
        }
        else
        {
            public_decrypt(rsa, instr, outstr, mode);
            RSA_free(rsa);
            BIO_free(keybio);
        }
    }
    catch (...)
    {
        if (rsa != NULL)
            RSA_free(rsa);
        if (keybio != NULL)
            BIO_free(keybio);
        throw;
    }
}

void CRSAHelper::CRSAHelper::public_encrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    int rsalen = 0;
    int outlen = 0;
    RSA* rsa_ = (RSA*)rsa;

    // Returns the RSA modulus size in bytes.
    // It can be used to determine how much memory must be allocated for an RSA encrypted value.
    // rsa->n must not be NULL.
    // RSA_size() is available in all versions of SSLeay and OpenSSL.
    rsalen = RSA_size(rsa_);
    outstr->resize(rsalen);
    if (mode == PKCS1_PADDING)
        rsalen = rsalen - 11;
    else if (mode == PKCS1_OAEP_PADDING)
        rsalen = rsalen - 41;

    // RSA_public_encrypt() encrypts the flen bytes at
    // from (usually a session key) using the public key rsa and stores the ciphertext in to.
    // to must point to RSA_size(rsa) bytes of memory.
    //
    // flen must be less than RSA_size(rsa) - 11 for the PKCS #1 v1.5 based padding modes,
    // less than RSA_size(rsa) - 41 for RSA_PKCS1_OAEP_PADDING and exactly RSA_size(rsa) for RSA_NO_PADDING.
    // The random number generator must be seeded prior to calling RSA_public_encrypt().
    //
    // RSA_public_encrypt() returns the size of the encrypted data (i.e., RSA_size(rsa)).
    // On error, -1 is returned; the error codes can be obtained by err_get_error(3).
    //
    // https://linux.die.net/man/3/rsa_public_encrypt
    //
    // int RSA_public_encrypt(int flen, unsigned char *from, unsigned char *to, RSA *rsa, int padding);
    outlen = RSA_public_encrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa_, mode);
    if (outlen != -1)
    {
        // SUCCESS
        outstr->resize(outlen);
    }
    else
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }
}

void CRSAHelper::private_decrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    int rsalen = 0;
    int outlen = 0;
    RSA* rsa_ = (RSA*)rsa;

    // Returns the RSA modulus size in bytes.
    // It can be used to determine how much memory must be allocated for an RSA encrypted value.
    // rsa->n must not be NULL.
    // RSA_size() is available in all versions of SSLeay and OpenSSL.
    rsalen = RSA_size(rsa_);
    outstr->resize(rsalen);
    if (mode == PKCS1_PADDING)
        rsalen = rsalen - 11;
    else if (mode == PKCS1_OAEP_PADDING)
        rsalen = rsalen - 41;

    // RSA_private_decrypt() decrypts the flen bytes at from using the private key rsa and stores the plaintext in to.
    // to must point to a memory section large enough to hold the decrypted data (which is smaller than RSA_size(rsa)).
    // padding is the padding mode that was used to encrypt the data.
    //
    // RSA_private_decrypt() returns the size of the recovered plaintext.
    // On error, -1 is returned; the error codes can be obtained by err_get_error(3).
    outlen = RSA_private_decrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa_, mode);
    if (outlen != -1)
    {
        // SUCCESS
        outstr->resize(outlen);
    }
    else
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }
}

void CRSAHelper::private_encrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    int rsalen = 0;
    int outlen = 0;
    RSA* rsa_ = (RSA*)rsa;

    // Returns the RSA modulus size in bytes.
    // It can be used to determine how much memory must be allocated for an RSA encrypted value.
    // rsa->n must not be NULL.
    // RSA_size() is available in all versions of SSLeay and OpenSSL.
    rsalen = RSA_size(rsa_);
    outstr->resize(rsalen);
    if (mode == PKCS1_PADDING)
        rsalen = rsalen - 11;
    else if (mode == PKCS1_OAEP_PADDING)
        rsalen = rsalen - 41;

    // RSA_private_encrypt() signs the flen bytes at from (usually a message digest with an algorithm identifier)
    // using the private key rsa and stores the signature in to.
    // to must point to RSA_size(rsa) bytes of memory.
    //
    // RSA_private_encrypt() returns the size of the signature (i.e., RSA_size(rsa)).
    // On error, -1 is returned; the error codes can be obtained by err_get_error(3).
    outlen = RSA_private_encrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa_, mode);
    if (outlen != -1)
    {
        outstr->resize(outlen);
    }
    else
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }
}

void CRSAHelper::public_decrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode)
{
    int errcode = 0;
    int rsalen = 0;
    int outlen = 0;
    RSA* rsa_ = (RSA*)rsa;

    // Returns the RSA modulus size in bytes.
    // It can be used to determine how much memory must be allocated for an RSA encrypted value.
    // rsa->n must not be NULL.
    // RSA_size() is available in all versions of SSLeay and OpenSSL.
    rsalen = RSA_size(rsa_);
    outstr->resize(rsalen);
    if (mode == PKCS1_PADDING) // RSA_PKCS1_PADDING
        rsalen = rsalen - 11;
    else if (mode == PKCS1_OAEP_PADDING) // RSA_PKCS1_OAEP_PADDING
        rsalen = rsalen - 41;

    // RSA_public_decrypt() recovers the message digest from the flen bytes long signature at
    // from using the signer's public key rsa. to must point to a memory section large enough to
    // hold the message digest (which is smaller than RSA_size(rsa) - 11).
    // padding is the padding mode that was used to sign the data.
    //
    // RSA_public_decrypt() returns the size of the recovered message digest.
    // On error, -1 is returned; the error codes can be obtained by err_get_error(3).
    outlen = RSA_public_decrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa_, mode);
    if (outlen != -1)
    {
        outstr->resize(outlen);
    }
    else
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }
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

bool init_private_key_from_str(void** pkey, const std::string& private_key_str, std::string* errmsg)
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
