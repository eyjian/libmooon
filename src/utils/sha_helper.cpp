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
#include "mooon/utils/sha_helper.h"
#include "mooon/utils/scoped_ptr.h"
#if MOOON_HAVE_OPENSSL == 1
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
UTILS_NAMESPACE_BEGIN

SHAType int2shatype(int n)
{
    switch (n)
    {
    case 1:
        return SHA1;
    case 224:
        return SHA224;
    case 256:
        return SHA256;
    case 384:
        return SHA384;
    case 512:
        return SHA512;
    default:
        return SHA0;
    }
}

std::string CSHAHelper::sha(SHAType sha_type, const char* format, ...)
{
	int expected = 0;
    size_t size = 1024;
    ScopedArray<char> buffer(new char[size]);
    va_list ap;

    while (true)
    {
        va_start(ap, format);
        expected = vsnprintf(buffer.get(), size, format, ap);
        va_end(ap);

        if (expected > -1 && expected < (int)size)
            break;

        if (expected > -1)    /* glibc 2.1 */
            size = expected + 1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */

        buffer.reset(new char[size]);
    }

    CSHAHelper sha_helper(sha_type);
    sha_helper.update(buffer.get(), expected);
    return sha_helper.to_string();
}

std::string CSHAHelper::lowercase_sha(SHAType sha_type, const char* format, ...)
{
	int expected = 0;
    size_t size = 1024;
    ScopedArray<char> buffer(new char[size]);
    va_list ap;

    while (true)
    {
        va_start(ap, format);
        expected = vsnprintf(buffer.get(), size, format, ap);
        va_end(ap);

        if (expected > -1 && expected < (int)size)
            break;

        if (expected > -1)    /* glibc 2.1 */
            size = expected + 1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */

        buffer.reset(new char[size]);
    }

    CSHAHelper sha_helper(sha_type);
    sha_helper.update(buffer.get(), expected);
    return sha_helper.to_string(false);
}

std::string CSHAHelper::uppercase_sha(SHAType sha_type, const char* format, ...)
{
	int expected = 0;
    size_t size = 1024;
    ScopedArray<char> buffer(new char[size]);
    va_list ap;

    while (true)
    {
        va_start(ap, format);
        expected = vsnprintf(buffer.get(), size, format, ap);
        va_end(ap);

        if (expected > -1 && expected < (int)size)
            break;

        if (expected > -1)    /* glibc 2.1 */
            size = expected + 1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */

        buffer.reset(new char[size]);
    }

    CSHAHelper sha_helper(sha_type);
    sha_helper.update(buffer.get(), expected);
    return sha_helper.to_string(true);
}

// SHA1算法是对MD5算法的升级,计算结果为20字节(160位)
CSHAHelper::CSHAHelper(SHAType sha_type)
    : _sha_type(sha_type)
{
    switch (_sha_type)
    {
        case SHA224:
        {
            SHA256_CTX* sha_context = new SHA256_CTX;
            SHA224_Init(sha_context);
            _sha_context = sha_context;
            break;
        }
        case SHA256:
        {
            SHA256_CTX* sha_context = new SHA256_CTX;
            SHA256_Init(sha_context);
            _sha_context = sha_context;
            break;
        }
        case SHA384:
        {
            SHA512_CTX* sha_context = new SHA512_CTX;
            SHA384_Init(sha_context);
            _sha_context = sha_context;
            break;
        }
        case SHA512:
        {
            SHA512_CTX* sha_context = new SHA512_CTX;
            SHA512_Init(sha_context);
            _sha_context = sha_context;
            break;
        }
        default:
        {
            SHA_CTX* sha_context = new SHA_CTX;
            SHA1_Init(sha_context);
            _sha_context = sha_context;
            break;
        }
    }
}

CSHAHelper::~CSHAHelper()
{
    switch (_sha_type)
    {
        case SHA224:
        {
            delete (SHA256_CTX*)_sha_context;
            break;
        }
        case SHA256:
        {
            delete (SHA256_CTX*)_sha_context;
            break;
        }
        case SHA384:
        {
            delete (SHA512_CTX*)_sha_context;
            break;
        }
        case SHA512:
        {
            delete (SHA512_CTX*)_sha_context;
            break;
        }
        default:
        {
            delete (SHA_CTX*)_sha_context;
            break;
        }
    }
}

void CSHAHelper::update(const char* str, size_t size)
{
    switch (_sha_type)
    {
        case SHA224:
        {
            SHA256_CTX* sha_context = (SHA256_CTX*)_sha_context;
            SHA224_Update(sha_context, str, size);
            break;
        }
        case SHA256:
        {
            SHA256_CTX* sha_context = (SHA256_CTX*)_sha_context;
            SHA256_Update(sha_context, str, size);
            break;
        }
        case SHA384:
        {
            SHA512_CTX* sha_context = (SHA512_CTX*)_sha_context;
            SHA384_Update(sha_context, str, size);
            break;
        }
        case SHA512:
        {
            SHA512_CTX* sha_context = (SHA512_CTX*)_sha_context;
            SHA512_Update(sha_context, str, size);
            break;
        }
        default:
        {
            SHA_CTX* sha_context = (SHA_CTX*)_sha_context;
            SHA1_Update(sha_context, str, size);
            break;
        }
    }
}

void CSHAHelper::update(const std::string& str)
{
	update(str.c_str(), str.size());
}

// Why does OPENSSL_cleanse look so complex and thread-unsafe?
//
// unsigned char cleanse_ctr = 0;
//
// void OPENSSL_cleanse(void *ptr, size_t len)
// {
//     unsigned char *p = ptr;
//     size_t loop = len, ctr = cleanse_ctr;
//     while(loop--)
//     {
//         *(p++) = (unsigned char)ctr;
//         ctr += (17 + ((size_t)p & 0xF));
//     }
//     p=memchr(ptr, (unsigned char)ctr, len);
//     if(p)
//         ctr += (63 + (size_t)p);
//     cleanse_ctr = (unsigned char)ctr;
// }
//
// openssl-1.1.0f/mem_clr.c:
//
// Pointer to memset is volatile so that compiler must de-reference
// the pointer and can't assume that it points to any function in
// particular (such as memset, which it then might further "optimize")
// typedef void *(*memset_t)(void *, int, size_t);
//
// static volatile memset_t memset_func = memset;
//
// void OPENSSL_cleanse(void *ptr, size_t len)
// {
    //     memset_func(ptr, 0, len);
// }
void CSHAHelper::to_string(std::string* str, bool uppercase) const
{
    int DIGEST_LENGTH;
    unsigned char* digest = NULL;

    switch (_sha_type)
    {
        case SHA224:
        {
            SHA256_CTX sha_context;
            memcpy(&sha_context, _sha_context, sizeof(SHA256_CTX));

            DIGEST_LENGTH = SHA224_DIGEST_LENGTH;
            digest = new unsigned char[DIGEST_LENGTH];
            SHA224_Final(digest, &sha_context); // SHA224_Final会修改sha_context
            // OPENSSL_cleanse() fills ptr of size len with a string of 0's
            // Use OPENSSL_cleanse() with care if the memory is a mapping of a file.
            // void OPENSSL_cleanse(void *ptr, size_t len);
            //OPENSSL_cleanse(&sha_context, sizeof(SHA256_CTX));
            break;
        }
        case SHA256:
        {
            SHA256_CTX sha_context;
            memcpy(&sha_context, _sha_context, sizeof(SHA256_CTX));

            DIGEST_LENGTH = SHA256_DIGEST_LENGTH;
            digest = new unsigned char[DIGEST_LENGTH];
            SHA256_Final(digest, &sha_context);
            //OPENSSL_cleanse(&sha_context, sizeof(SHA256_CTX));
            break;
        }
        case SHA384:
        {
            SHA512_CTX sha_context;
            memcpy(&sha_context, _sha_context, sizeof(SHA512_CTX));

            DIGEST_LENGTH = SHA384_DIGEST_LENGTH;
            digest = new unsigned char[DIGEST_LENGTH];
            SHA384_Final(digest, &sha_context);
            //OPENSSL_cleanse(&sha_context, sizeof(SHA512_CTX));
            break;
        }
        case SHA512:
        {
            SHA512_CTX sha_context;
            memcpy(&sha_context, _sha_context, sizeof(SHA512_CTX));

            DIGEST_LENGTH = SHA512_DIGEST_LENGTH;
            digest = new unsigned char[DIGEST_LENGTH];
            SHA512_Final(digest, &sha_context);
            //OPENSSL_cleanse(&sha_context, sizeof(SHA512_CTX));
            break;
        }
        default:
        {
            SHA_CTX sha_context;
            memcpy(&sha_context, _sha_context, sizeof(SHA_CTX));

            DIGEST_LENGTH = SHA_DIGEST_LENGTH;
            digest = new unsigned char[DIGEST_LENGTH];
            SHA1_Final(digest, &sha_context);
            //OPENSSL_cleanse(&sha_context, sizeof(SHA_CTX));
            break;
        }
    }

    str->resize(2*DIGEST_LENGTH);
    char* str_p = const_cast<char*>(str->data());
    for (int i=0; i<DIGEST_LENGTH; ++i)
    {
        if (uppercase)
            (void)snprintf(str_p + i*2, 3, "%02X", digest[i]);
        else
            (void)snprintf(str_p + i*2, 3, "%02x", digest[i]);
    }
    delete []digest;
}

std::string CSHAHelper::to_string(bool uppercase) const
{
    std::string str;
	to_string(&str, uppercase);
    return str;
}

static std::string to_hex(unsigned char *data, size_t size, bool uppercase)
{
    std::string hex;
    for (size_t i = 0; i < size; ++i) {
        char buf[3];
        if (uppercase)
            sprintf(buf, "%02X", data[i]);
        else
            sprintf(buf, "%02x", data[i]);
        hex += buf;
    }
    return hex;
}

static std::string hmac_sha256(const std::string &key, const std::string &data, bool uppercase)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    HMAC_CTX hmac;
    HMAC_CTX_init(&hmac);
    HMAC_Init_ex(&hmac, &key[0], key.length(), EVP_sha256(), NULL);
    HMAC_Update(&hmac, (unsigned char*)&data[0], data.length());
    unsigned int len = SHA256_DIGEST_LENGTH;
    HMAC_Final(&hmac, hash, &len);
    HMAC_CTX_cleanup(&hmac);
    return to_hex(hash, len, uppercase);
}

std::string lowercase_hmac_sha256(const std::string &key, const std::string &data)
{
    return hmac_sha256(key, data, false);
}

std::string uppercase_hmac_sha256(const std::string &key, const std::string &data)
{
    return hmac_sha256(key, data, true);
}

UTILS_NAMESPACE_END
#endif // MOOON_HAVE_OPENSSL
