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
#include "mooon/utils/scoped_ptr.h"
#if MOOON_HAVE_OPENSSL == 1
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
UTILS_NAMESPACE_BEGIN

// 参考：
// http://hayageek.com/rsa-encryption-decryption-openssl-c/
void CRSAHelper::public_encrypt(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag)
{
    int errcode = 0;
    int rsalen = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(pub_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open %s failed: %s", pub_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read %s failed: %s", pub_keyfile.c_str(), buf.get()), errcode);
    }

    rsalen = RSA_size(rsa);
    outstr->resize(rsalen + 1);
    if (-1 == RSA_public_encrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa, flag))
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        RSA_free(rsa);
        THROW_EXCEPTION(utils::CStringUtils::format_string("encrypt %s failed: %s", pub_keyfile.c_str(), buf.get()), errcode);
    }

    fclose(fp);
    RSA_free(rsa);
}

void CRSAHelper::private_decrypt(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag)
{
    int errcode = 0;
    int rsalen = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(priv_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open %s failed: %s", priv_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read %s failed: %s", priv_keyfile.c_str(), buf.get()), errcode);
    }

    rsalen = RSA_size(rsa);
    outstr->resize(rsalen + 1);
    if (-1 == RSA_private_decrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa, flag))
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        RSA_free(rsa);
        THROW_EXCEPTION(utils::CStringUtils::format_string("encrypt %s failed: %s", priv_keyfile.c_str(), buf.get()), errcode);
    }

    fclose(fp);
    RSA_free(rsa);
}

void CRSAHelper::private_encrypt(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag)
{
    int errcode = 0;
    int rsalen = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(priv_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open %s failed: %s", priv_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read %s failed: %s", priv_keyfile.c_str(), buf.get()), errcode);
    }

    rsalen = RSA_size(rsa);
    outstr->resize(rsalen + 1);
    if (-1 == RSA_private_encrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa, flag))
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        RSA_free(rsa);
        THROW_EXCEPTION(utils::CStringUtils::format_string("encrypt %s failed: %s", priv_keyfile.c_str(), buf.get()), errcode);
    }

    fclose(fp);
    RSA_free(rsa);
}

void CRSAHelper::public_decrypt(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag)
{
    int errcode = 0;
    int rsalen = 0;
    RSA* rsa = NULL;
    FILE* fp = fopen(pub_keyfile.c_str(), "r");

    if (fp == NULL)
    {
        errcode = errno;
        THROW_EXCEPTION(utils::CStringUtils::format_string("open %s failed: %s", pub_keyfile.c_str(), strerror(errcode)), errcode);
    }
    rsa = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if (rsa == NULL)
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        THROW_EXCEPTION(utils::CStringUtils::format_string("read %s failed: %s", pub_keyfile.c_str(), buf.get()), errcode);
    }

    rsalen = RSA_size(rsa);
    outstr->resize(rsalen + 1);
    if (-1 == RSA_public_decrypt(rsalen, (unsigned char *)instr.data(), (unsigned char*)outstr->data(), rsa, flag))
    {
        errcode = ERR_get_error();
        utils::ScopedArray<char> buf(new char[SIZE_4K]);
        ERR_error_string_n(errcode, buf.get(), SIZE_4K);
        fclose(fp);
        RSA_free(rsa);
        THROW_EXCEPTION(utils::CStringUtils::format_string("encrypt %s failed: %s", pub_keyfile.c_str(), buf.get()), errcode);
    }

    fclose(fp);
    RSA_free(rsa);
}

UTILS_NAMESPACE_END
#endif // MOOON_HAVE_OPENSSL

