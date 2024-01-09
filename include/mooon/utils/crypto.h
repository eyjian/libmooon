// Writed by yijian on 2018/12/23
#ifndef MOOON_UTILS_CRYPTO_H
#define MOOON_UTILS_CRYPTO_H
#include "mooon/utils/config.h"
UTILS_NAMESPACE_BEGIN
#if MOOON_HAVE_OPENSSL == 1

void base64_encode(const std::string& src, std::string* dest, bool no_newline=true);
void base64_decode(const std::string& src, std::string* dest, bool no_newline=true);

// 释放私钥
void release_private_key(void** pkey);

// 从文件初始化私钥
// private_key_file 私钥文件
bool init_private_key_from_file(void** pkey, const std::string& private_key_file, std::string* errmsg);

// 从字符串初始化私钥
// private_key_str 私钥字符串
bool init_private_key(void** pkey, const std::string& private_key_str, std::string* errmsg);

// 基于 sha256 的 rsa 签名
// signature 签名值
// pkey 私钥
// data 需要签名的数据
// errmsg 签名失败时存储出错信息
bool RSA256_sign(std::string* signature, void* pkey, const std::string& data, std::string* errmsg);

#endif // MOOON_HAVE_OPENSSL
UTILS_NAMESPACE_END
#endif // MOOON_UTILS_CRYPTO_H
