// Copyright 2021, Tencent Inc.
// Author: yijian
// Date: 2021/9/4
// 特别说明：
// 多线程环境，应当在创建线程之前调用 CCurlWrapper::global_init() 一次，
// 在进程线程退出后进程退出前调用一次 CCurlWrapper::global_cleanup() 。
#ifndef GONGYI_HELPER_CURL_H
#define GONGYI_HELPER_CURL_H
#include <mooon/sys/curl_wrapper.h>
#include <memory>
namespace gongyi { namespace helper {

// 取得当前线程的 CCurlWrapper
//
// 参数说明：
// 对于CGI等单线程程序，nosignal为false即可，并且也不用指定c-ares，
// 但对于多线程使用curl的程序，则nosignal应当设定为true，
// 指定nosignal为true时，DNS解析将不带超时 。
std::shared_ptr<mooon::sys::CCurlWrapper> get_thread_curl(bool nosignal=false);

}} // namespace gongyi { namespace helper {
#endif // GONGYI_HELPER_CURL_H
