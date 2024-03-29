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
 * Author: eyjian@qq.com or eyjian@gmail.com
 */
#ifndef MOOON_SYS_DATETIME_UTILS_H
#define MOOON_SYS_DATETIME_UTILS_H
#include "mooon/sys/utils.h"
#include <time.h>
#include <vector>
SYS_NAMESPACE_BEGIN

/** 日期时间工具类
  */
class CDatetimeUtils
{
public:
    /** 判断是否为同一天 */
    static bool is_same_day(time_t t1, time_t t2);

    /** 判断指定年份是否为闰年 */
    static bool is_leap_year(int year);
    
    /** 将time_t值转换成类似于20160114这样的整数 */
    static uint32_t time2date(time_t t);

    /** 将tm值转换成time_t值 */
    static time_t tm2time_t(const struct tm& tm);

    /*
     * 求和datetime相隔的日期，比如days为1时表示date的后一天，days为-1表示date的前一天等
     * 调用者需要确保date格式为有效的日期，并满足格式YYYY-MM-DD hh:mm:ss，否则返回false
     */
    static bool neighbor_date_bytime(const std::string& datetime, int days, std::string* neighbor_date);
    /** 如果date为无效日期则返回空 */
    static std::string neighbor_date_bytime(const std::string& datetime, int days);

    /*
     * 求和date相隔的日期，比如days为1时表示date的后一天，days为-1表示date的前一天等
     * 调用者需要确保date格式为有效的日期，并满足格式YYYY-MM-DD，否则返回false
     */
    static bool neighbor_date_bydate(const std::string& date, int days, std::string* neighbor_date);
    /** 如果date为无效日期则返回空 */
    static std::string neighbor_date_bydate(const std::string& date, int days);

    /*
     * 求邻居月份
     * 如果date是一个不符号“YYYY-MM-DD”的值则返回空字符串，否则返回格式为“YYYY-MM-01”的月份字符串
     */
    static std::string neighbor_month_bydate(const std::string& date, int months);

    // 输入YYYY-MM-DD hh:mm:ss，返回YYYY-MM-DD，如输入"2016-09-07 09:58:12"，返回"2016-09-07"
    // 需要使用者确保输入符合格式YYYY-MM-DD hh:mm:ss，函数内部不做检查
    static std::string extract_date(const char* datetime);
    static std::string extract_date(const std::string& datetime);

    // 输入YYYY-MM-DD hh:mm:ss，返回hh:mm:ss，如输入"2016-09-07 09:58:12"，返回"09:58:12"
    // 需要使用者确保输入符合格式YYYY-MM-DD hh:mm:ss，函数内部不做检查
    static std::string extract_time(const char* datetime);
    static std::string extract_time(const std::string& datetime);

    // 输入YYYY-MM-DD hh:mm:ss，参数date将存放YYYY-MM-DD，而time参数存放hh:mm:ss
    // 需要使用者确保输入符合格式YYYY-MM-DD hh:mm:ss，函数内部不做检查
    static void extract_datetime(const char* datetime, std::string* date, std::string* time);
    static void extract_datetime(const std::string& datetime, std::string* date, std::string* time);

    // 不对date做检查，调用者需要保证它的格式以“YYYY-MM”打头
    // 提取月，返回格式为：YYYY-MM，假设date为2017-10-30，则返回2017-10
    static std::string extract_month(const std::string& date);

    // 不对date做检查，调用者需要保证它的格式以“YYYY”打头
    // 提取年，返回格式为：YYYY，假设date为2017-10-30，则返回2017
    static std::string extract_year(const std::string& date);

    // 不对date做检查，调用者需要保证它的格式以“YYYY-MM”打头
    // 提取月，返回格式为：YYYY-MM-01，假设date为2017-10-30，则返回2017-10-01
    static std::string extract_standard_month(const std::string& date);

    // 不对date做检查，调用者需要保证它的格式以“YYYY”打头
    // 提取年，返回格式为：YYYY-01-01，假设date为2017-10-30，则返回2017-01-01
    static std::string extract_standard_year(const std::string& date);

    /** 得到当前日期和时间，返回格式由参数format决定，默认为: YYYY-MM-DD HH:SS:MM
      * @datetime_buffer: 用来存储当前日期和时间的缓冲区
      * @datetime_buffer_size: datetime_buffer的字节大小，不应当于小“YYYY-MM-DD HH:SS:MM”
      */
    static void get_current_datetime(char* datetime_buffer, size_t datetime_buffer_size, const char* format="%04d-%02d-%02d %02d:%02d:%02d");
    static std::string get_current_datetime(const char* format="%04d-%02d-%02d %02d:%02d:%02d");

    /** 得到当前日期，返回格式由参数format决定，默认为: YYYY-MM-DD
      * @date_buffer: 用来存储当前日期的缓冲区
      * @date_buffer_size: date_buffer的字节大小，不应当于小“YYYY-MM-DD”
      */
    static void get_current_date(char* date_buffer, size_t date_buffer_size, const char* format="%04d-%02d-%02d");
    static std::string get_current_date(const char* format="%04d-%02d-%02d");

    /** 得到当前时间，返回格式由参数format决定，默认为: HH:SS:MM
      * @time_buffer: 用来存储当前时间的缓冲区
      * @time_buffer_size: time_buffer的字节大小，不应当于小“HH:SS:MM”
      */
    static void get_current_time(char* time_buffer, size_t time_buffer_size, const char* format="%02d:%02d:%02d");
    static std::string get_current_time(const char* format="%02d:%02d:%02d");

    /** 得到当前日期和时间结构
      * @current_datetime_struct: 指向当前日期和时间结构的指针
      */
    static void get_current_datetime_struct(struct tm* current_datetime_struct);

    /** 日期和时间 */
    static void to_current_datetime(const struct tm* current_datetime_struct, char* datetime_buffer, size_t datetime_buffer_size, const char* format="%04d-%02d-%02d %02d:%02d:%02d");
    static std::string to_current_datetime(const struct tm* current_datetime_struct, const char* format="%04d-%02d-%02d %02d:%02d:%02d");

    /** 仅日期 */
    static void to_current_date(const struct tm* current_datetime_struct, char* date_buffer, size_t date_buffer_size, const char* format="%04d-%02d-%02d");
    static std::string to_current_date(const struct tm* current_datetime_struct, const char* format="%04d-%02d-%02d");

    /** 仅时间 */
    static void to_current_time(const struct tm* current_datetime_struct, char* time_buffer, size_t time_buffer_size, const char* format="%02d:%02d:%02d");
    static std::string to_current_time(const struct tm* current_datetime_struct, const char* format="%02d:%02d:%02d");

    /** 仅年份 */
    static void to_current_year(const struct tm* current_datetime_struct, char* year_buffer, size_t year_buffer_size);
    static std::string to_current_year(const struct tm* current_datetime_struct);

    /** 仅月份 */
    static void to_current_month(const struct tm* current_datetime_struct, char* month_buffer, size_t month_buffer_size);
    static std::string to_current_month(const struct tm* current_datetime_struct);

    /** 仅天 */
    static void to_current_day(const struct tm* current_datetime_struct, char* day_buffer, size_t day_buffer_size);
    static std::string to_current_day(const struct tm* current_datetime_struct);

    /** 仅小时 */
    static void to_current_hour(const struct tm* current_datetime_struct, char* hour_buffer, size_t hour_buffer_size);
    static std::string to_current_hour(const struct tm* current_datetime_struct);

    /** 仅分钟 */
    static void to_current_minite(const struct tm* current_datetime_struct, char* minite_buffer, size_t minite_buffer_size);
    static std::string to_current_minite(const struct tm* current_datetime_struct);

    /** 仅秒钟 */
    static void to_current_second(const struct tm* current_datetime_struct, char* second_buffer, size_t second_buffer_size);
    static std::string to_current_second(const struct tm* current_datetime_struct);

    /***
      * 将一个字符串转换成日期时间格式
      * @str: 符合“YYYY-MM-DD HH:MM:SS”格式的日期时间
      * @datetime_struct: 存储转换后的日期时间
      * @isdst: 值大于0时表示为夏令时，值为0表示非夏令时，值为负数表示未知，注意大于0的值严重影响性能
      * @return: 转换成功返回true，否则返回false
      */
    static bool datetime_struct_from_string(const char* str, struct tm* datetime_struct, int isdst=0);
    static bool datetime_struct_from_string(const char* str, time_t* datetime, int isdst=0);

    // 返回“YYYY-MM-DD HH:MM:SS”格式的日期时间
    static std::string to_string(time_t datetime, const char* format="%04d-%02d-%02d %02d:%02d:%02d");

    // 返回格式由参数format决定，默认为“YYYY-MM-DD HH:MM:SS”格式的日期时间
    static std::string to_datetime(time_t datetime, const char* format="%04d-%02d-%02d %02d:%02d:%02d");

    // 返回格式由参数format决定，默认为“YYYY-MM-DD”格式的日期时间
    static std::string to_date(time_t datetime, const char* format="%04d-%02d-%02d");

    // 返回格式由参数format决定，默认为“HH:MM:SS”格式的日期时间
    static std::string to_time(time_t datetime, const char* format="%02d:%02d:%02d");

    // 得到当前的毫秒值
    static uint64_t get_current_milliseconds();

    // 得到当前的微秒值
    static uint64_t get_current_microseconds();
};

// 是否为有效的日期时间，str格式要求为“YYYY-MM-DD hh:mm:ss”
extern bool is_valid_datetime(const std::string& str);

// 是否为有效的日期，str格式要求为“YYYY-MM-DD”
extern bool is_valid_date(const std::string& str);

// 是否为有效的时间，str格式要求为“hh:mm:ss”
extern bool is_valid_time(const std::string& str);

// 返回从1970-01-01 00:00:00以来的秒，返回值和time(NULL)的返回值相等
extern uint64_t current_seconds();

// 返回从1970-01-01 00:00:00以来的毫秒
extern uint64_t current_milliseconds();

// 返回今天
extern std::string today(const char* format="%04d-%02d-%02d");
// 返回昨天
extern std::string yesterday(const char* format="%04d-%02d-%02d");
// 返回明天
extern std::string tomorrow(const char* format="%04d-%02d-%02d");

// 取得格式化的当前日期时间，
// 如果with_microseconds为true，则返回格式为：YYYY-MM-DD hh:mm:ss/ms，其中ms最长为10位数字；
// 如果with_microseconds为false，则返回格式为：YYYY-MM-DD hh:mm:ss。
//
// 如果with_microseconds为true则datetime_buffer_size的大小不能小于sizeof("YYYY-MM-DD hh:mm:ss/0123456789")，
// 如果with_microseconds为false则datetime_buffer_size的大小不能小于sizeof("YYYY-MM-DD hh:mm:ss")，
extern void get_formatted_current_datetime(char* datetime_buffer, size_t datetime_buffer_size, bool with_microseconds=true);

// 如果with_microseconds为false，则返回同CDatetimeUtils::get_current_datetime()
// 如果with_microseconds为true，则返回为：YYYY-MM-DD hh:mm:ss/microseconds
extern std::string get_formatted_current_datetime(bool with_microseconds=true);

// 格式为YYYY-MM-DD转成格式为YYYYMMDD的无符号4字节整数值，
// 如果date是一个无效的值，则返回值为0
extern uint32_t date2day(const std::string& date);

// 格式为YYYY-MM-DD转成格式为YYYYMM的无符号4字节整数值，
// 如果date是一个无效的值，则返回值为0
extern uint32_t date2month(const std::string& date);

// 格式为YYYY-MM-DD转成格式为YYYY的无符号4字节整数值，
// 如果date是一个无效的值，则返回值为0
extern uint32_t date2year(const std::string& date);

SYS_NAMESPACE_END
#endif // MOOON_SYS_DATETIME_UTILS_H
