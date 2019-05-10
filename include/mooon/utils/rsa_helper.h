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
#include <stdarg.h>
#include <stdint.h>
UTILS_NAMESPACE_BEGIN

// RSA填充模式
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

// RSA是一种非对称加密算法
// 1977年由罗纳德·李维斯特（Ron Rivest）、阿迪·萨莫尔（Adi Shamir）和伦纳德·阿德曼（Leonard Adleman）一起提出的
// RSA是他们三人姓氏开头字母拼在一起组成
class CRSAHelper
{
public:
    // pub_keyfile 存储了公钥的文件
    // priv_keyfile 存储了私钥的文件
    // 如果出错，则抛出异常mooon::utils::CException
    static void public_encrypt_bykeyfile(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void private_decrypt_bykeyfile(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void private_encrypt_bykeyfile(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void public_decrypt_bykeyfile(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, RSAPaddingMode mode);

    static void public_encrypt_bykey(const std::string& pub_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void private_decrypt_bykey(const std::string& priv_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void private_encrypt_bykey(const std::string& priv_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void public_decrypt_bykey(const std::string& pub_key, const std::string& instr, std::string* outstr, RSAPaddingMode mode);

private:
    static void public_encrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void private_decrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void private_encrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
    static void public_decrypt(void* rsa, const std::string& instr, std::string* outstr, RSAPaddingMode mode);
};

UTILS_NAMESPACE_END
#endif // MOOON_UTILS_SHA_HELPER_H
