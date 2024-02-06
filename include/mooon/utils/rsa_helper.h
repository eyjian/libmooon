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
#ifndef MOOON_UTILS_RSA_HELPER_H
#define MOOON_UTILS_RSA_HELPER_H
#include "mooon/utils/exception.h"
#include "mooon/utils/string_utils.h"
#include <stdint.h>
UTILS_NAMESPACE_BEGIN

// RSA 填充模式
enum RSAPaddingMode
{
    // RSAES-PKCS1-v1_5/RSASSA-PKCS1-v1_5填充
    // PKCS #1 v1.5 padding. This currently is the most widely used mode.
    PKCS1_PADDING = 1,

    // SSLv23填充
    // PKCS #1 v1.5 padding with an SSL-specific modification that denotes that the server is SSL3 capable.
    SSLV23_PADDING = 2,

    // 不填充
    // Raw RSA encryption.
    // This mode should only be used to implement cryptographically sound padding modes
    // in the application code.
    // Encrypting user data directly with RSA is insecure.
    NO_PADDING = 3,

    // RSAES-OAEP填充，强制使用SHA1（加密使用）
    // EME-OAEP as defined in PKCS #1 v2.0 with SHA-1 , MGF1 and an empty encoding parameter.
    // This mode is recommended for all new applications.
    PKCS1_OAEP_PADDING = 4,

    // X9.31填充（签名使用）
    X931_PADDING = 5,

    // RSASSA-PSS填充（签名使用）
    PKCS1_PSS_PADDING = 6
};

// RSA 是一种非对称加密算法（用公钥加密的数据，只有对应的私钥可解密；用私钥加密的数据，只有对应的公钥可解密）
// 1977 年由罗纳德·李维斯特（Ron Rivest）、阿迪·萨莫尔（Adi Shamir）和伦纳德·阿德曼（Leonard Adleman）一起提出的
// RSA 是他们三人姓氏开头字母拼在一起组成
// RSA 加解密有长度限制，不超过 117 字节，超过需要采取分段加解密
class CRSAHelper
{
public:
    // private_key_filepath 私钥文件
    CRSAHelper(const std::string &private_key_filepath);
    ~CRSAHelper(); // 释放 pkey 等资源

public:
    void init(); // 失败抛 mooon::utils::CException 异常
    void release();
    void* pkey() { return _pkey; }
    void* pkey_ctx() { return _pkey_ctx; }

public: // 失败抛 mooon::utils::CException 异常
    // RSA 解密，本函数不做 base64 解码，需调用者 base64 解码后再调用
    void rsa_decrypt(std::string* decrypted_data, const std::string& base64_encrypted_data, void* pkey, void* ctx);

private:
    const std::string _private_key_filepath;
    FILE* _pkey_fp;
    void* _pkey;
    void* _pkey_ctx;
};

void* pkey2rsa(void* pkey);
void rsa256sign(std::string* signature_str, void* pkey, const std::string& data); // 失败抛 mooon::utils::CException 异常

UTILS_NAMESPACE_END
#endif // MOOON_UTILS_SHA_HELPER_H
