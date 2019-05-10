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

enum PaddingFlag
{
    PKCS1_PADDING = 1,
    SSLV23_PADDING = 2,
    NO_PADDING = 3
};

// RSA加密算法是一种非对称加密算法
// 1977年由罗纳德·李维斯特（Ron Rivest）、阿迪·萨莫尔（Adi Shamir）和伦纳德·阿德曼（Leonard Adleman）一起提出的
// RSA是他们三人姓氏开头字母拼在一起组成
class CRSAHelper
{
public:
    // pub_keyfile 存储了公钥的文件
    // priv_keyfile 存储了私钥的文件
    // 如果出错，则抛出异常mooon::utils::CException
    static void public_encrypt(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag=NO_PADDING);
    static void private_decrypt(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag=NO_PADDING);
    static void private_encrypt(const std::string& priv_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag=NO_PADDING);
    static void public_decrypt(const std::string& pub_keyfile, const std::string& instr, std::string* outstr, PaddingFlag flag=NO_PADDING);
};

UTILS_NAMESPACE_END
#endif // MOOON_UTILS_SHA_HELPER_H
