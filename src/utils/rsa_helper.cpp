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
#include "mooon/sys/log.h"
#include "mooon/utils/rsa_helper.h"
#include "mooon/utils/crypto.h"
#include "mooon/utils/scoped_ptr.h"
#include <string>
#include <vector>

#if MOOON_HAVE_OPENSSL == 1
#include <openssl/crypto.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
UTILS_NAMESPACE_BEGIN

static EVP_PKEY* get_pkey(void* pkey)
{
    return (EVP_PKEY*)pkey;
}

static EVP_PKEY_CTX* get_pkey_ctx(void *ctx)
{
    return (EVP_PKEY_CTX*)ctx;
}

static RSA* get_rsa(void* rsa)
{
    return (RSA*)rsa;
}

void* pkey2rsa(void* pkey)
{
    // 获取 RSA 私钥
    return EVP_PKEY_get1_RSA(get_pkey(pkey));
}

CRSAHelper::CRSAHelper(const std::string& private_key_filepath)
    : _private_key_filepath(private_key_filepath),
      _pkey_fp(nullptr),
      _pkey(nullptr),
      _pkey_ctx(nullptr)
{
}

CRSAHelper::~CRSAHelper()
{
    release();
}

void CRSAHelper::init()
{
    try
    {
        int errcode = 0;

        // _pkey_fp
        _pkey_fp = fopen(_private_key_filepath.c_str(), "rb");
        if (_pkey_fp == nullptr)
        {
            errcode = errno;
            THROW_EXCEPTION("error opening private key file", errcode);
        }

        // _pkey
        _pkey = PEM_read_PrivateKey(_pkey_fp, nullptr, nullptr, nullptr);
        if (_pkey == nullptr)
        {
            THROW_EXCEPTION("error reading private key file", -1);
        }

        // _pkey_ctx
        _pkey_ctx = EVP_PKEY_CTX_new(get_pkey(_pkey), nullptr);
        if (_pkey_ctx == nullptr)
        {
            THROW_EXCEPTION("error creating context", -1);
        }
    }
    catch (...)
    {
        release();
        throw;
    }
}

void CRSAHelper::release()
{
    if (_pkey_ctx != NULL)
    {
        EVP_PKEY_CTX_free(get_pkey_ctx(_pkey_ctx));
        _pkey_ctx = nullptr;
    }
    if (_pkey != nullptr)
    {
        EVP_PKEY_free(get_pkey(_pkey));
        _pkey = nullptr;
    }
    if (_pkey_fp != nullptr)
    {
        fclose(_pkey_fp);
        _pkey_fp = nullptr;
    }
}

// 本函数不做 base64 解码，需调用者 base64 解码后再调用
void CRSAHelper::rsa_decrypt(std::string* decrypted_data, const std::string& base64_encrypted_data, void* pkey, void* ctx)
{
    int errcode = 0;

    // 初始化解密操作，并没明确指定哈希算法
    if (EVP_PKEY_decrypt_init(get_pkey_ctx(_pkey_ctx)) <= 0)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }

    // 得到长度
    size_t decrypted_len = 0;

    // EVP_PKEY_decrypt 函数是一个通用的公钥解密函数，它可以处理多种加密算法，包括 RSA、DSA、DH 等，
    // 哈希算法的选择取决于在 EVP_PKEY_CTX 结构中设置的参数。
    // 哈希算法的选择是在创建 EVP_PKEY_CTX 时确定的，这通常是在调用 EVP_PKEY_CTX_new 或 EVP_PKEY_CTX_new_id 函数时完成的。
    // 这可以通过调用 EVP_PKEY_CTX_set_rsa_padding 和 EVP_PKEY_CTX_set_signature_md 函数来完成，如：
    // EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
    // EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256());
    // 这将确保在解密操作中使用 SHA-256 哈希算法，且需在调用 EVP_PKEY_decrypt_init 之前完成。
    if (EVP_PKEY_decrypt(get_pkey_ctx(ctx), nullptr, &decrypted_len, (unsigned char*)base64_encrypted_data.data(), base64_encrypted_data.size()) <= 0)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }

    // 解密数据
    decrypted_data->resize(decrypted_len, '\0');
    if (EVP_PKEY_decrypt(get_pkey_ctx(ctx), (unsigned char*)decrypted_data->data(), &decrypted_len, (unsigned char*)base64_encrypted_data.data(), base64_encrypted_data.size()) <= 0)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }
}

void rsa256sign(std::string* signature, void* pkey, const std::string& data)
{
    int errcode = 0;

    // 获取 RSA 私钥
    RSA* rsa = get_rsa(pkey2rsa(pkey));

    if (rsa == nullptr)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
        ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
        THROW_EXCEPTION(errmsg.get(), errcode);
    }
    else
    {
        // 签名内容
        unsigned char signature_bytes[EVP_PKEY_size(get_pkey(pkey))];
        size_t signature_len;
        EVP_MD_CTX* ctx = EVP_MD_CTX_create();
        EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, get_pkey(pkey));
        EVP_DigestSignUpdate(ctx, data.c_str(), data.length());
        if (EVP_DigestSignFinal(ctx, signature_bytes, &signature_len) != 1)
        {
            errcode = ERR_get_error();
            utils::ScopedArray<char> errmsg(new char[SIZE_4K]);
            ERR_error_string_n(errcode, errmsg.get(), SIZE_4K);
            THROW_EXCEPTION(errmsg.get(), errcode);
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
            mooon::utils::base64_encode(str, signature);
            BIO_free(bio);
        }
    }
}

UTILS_NAMESPACE_END
#endif // MOOON_HAVE_OPENSSL
