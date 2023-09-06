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
 * Author: jian yi, eyjian@qq.com
 */
#ifndef MOOON_SYS_UTILS_H
#define MOOON_SYS_UTILS_H
#include "mooon/sys/error.h"
#include "mooon/sys/syscall_exception.h"
#include <stdlib.h> // srand
#include <sys/time.h> // gettimeofday
#include <sys/types.h> // pid_t
#include <fstream>
#include <vector>
SYS_NAMESPACE_BEGIN

// 取得当前线程的 ID，
// 注意不同于 get_thread_self() 值。
// 相关的系统调用：tkill，tgkill 。
// 如果是主线程，则 gettid 和 getpid 返回相同的值。
pid_t gettid(void);

// 向指定线程发送指定的信号
// 如果 tgid 值为 -1 则等同 tkill
// 使用示例：tgkill(getpid(), gettid(), SIGTERM);
// 参数说明：
// 1) tgid 线程组 ID
// 2) tid 线程 ID
// 3) sig Linux 信号值
int tgkill(int tgid, int tid, int sig);

// 取得 fd 或 path 对应的 inode
// 返回值：失败返回 0，错误可通过 errno 取得
// ino_t 为 unsigned 类型
ino_t get_inode(int fd);
ino_t get_inode(const char *path);

#ifdef __GNUC__
    // 取得 ifs 对应的 inode
    // 返回值：失败返回 0
    ino_t get_inode(const std::ifstream& ifs);

    // 仅 GNU C++ 库可用
    // 返回值：失败返回 -1，错误可通过 errno 取得
    int ifstream2fd(const std::ifstream& fs);
    int ofstream2fd(const std::ofstream& fs);

    //fd2ifstream
    //__gnu_cxx::stdio_filebuf<char> fd_file_buf(fd, ios_base::in);
    //std::istream ifs(&fd_file_buf);
    //
    //fd2ofstream
    //__gnu_cxx::stdio_filebuf<char> fd_file_buf(fd, std::ios_base::out | std::ios_base::binary);
    //std::ostream ofs(&fd_file_buf);
#endif // __GNUC__

/***
  * 与系统调用有关的工具类函数实现
  */
class CUtils
{
public:
    // 由于rdtsc跟CPU核相关，
    // 因此使用时为得到准确数据，需要线程和CPU核心建立亲和关系
    static uint64_t rdtsc();

    // 基于nanosleep实现的sleep，保证至少睡眠milliseconds指定的时长
    static void millisleep(uint32_t milliseconds);

    // 基于nanosleep实现的sleep，保证至少睡眠microseconds指定的时长
    static void microsleep(uint32_t microseconds);

    // 基于poll实现的sleep，睡眠时长可能小于或微大于milliseconds指定的时长
    // 如果milliseconds为负值，则表示无限超时
    static void pollsleep(int milliseconds);

    // 基于select实现的sleep，保证至少睡眠milliseconds指定的时长
    static void selectsleep(int milliseconds);

    /** 得到指定系统调用错误码的字符串错误信息
      * @errcode: 系统调用错误码
      * @return: 系统调用错误信息
      */
    static std::string get_error_message(int errcode);

    /** 得到最近一次的出错信息 */
    static std::string get_last_error_message();

    /** 得到最近一次的出错代码 */
    static int get_last_error_code();

    /** 得到当前进程号 */
    static uint32_t get_current_process_id();

    /** 得到当前用户ID */
    static uint32_t get_current_userid();

    /** 得到当前用户名 */
    static std::string get_current_username();

    /** 得到当前进程所属可执行文件所在的全路径，包括文件名部分 */
    static std::string get_program_fullpath();

    /** 得到当前进程所属可执行文件所在的绝对路径，不包括文件名部分，并且结尾符不含反斜杠 */
    static std::string get_program_dirpath();
    static std::string get_program_path(); // 新的代码请使用get_program_dirpath，不要使用get_program_path
    
    /**
     * 取得指定进程的命令行参数
     * pid 如果为0表示取当前进程的
     * parameters 存储命令行参数
     *
     * 返回命令行参数个数
     */
    static int get_program_parameters(std::vector<std::string>* parameters, uint32_t pid=0);
    static std::string get_program_parameter0(uint32_t pid=0); // 第0个参数即为进程名

    /**
     * 取得指定进程的整个命令行
     * 假设运行命令：cat /proc/self/cmdline 1 2 3 4 5，
     * 那么返回结果将是：cat/proc/self/cmdline12345
     *
     * 如果pid值为0则表示取当前进程的
     * separator 指定各参数间的分隔字符
     */
    static std::string get_program_full_cmdline(char separator='|', uint32_t pid=0);

    /** 得到与指定fd相对应的文件名，包括路径部分
      * @fd: 文件描述符
      * @return: 文件名，包括路径部分，如果失败则返回空字符串
      */
	static std::string get_filename(int fd);

    /** 得到一个目录的绝对路径，路径中不会包含../和./等，是一个完整的原始路径
      * @directory: 目录的相对或绝对路径
      * @return: 返回目录的绝对路径，如果出错则返回空字符串
      */
    static std::string get_full_directory(const char* directory);

    /** 得到CPU核个数
      * @return: 如果成功，返回大于0的CPU核个数，否则返回0
      */
	static uint16_t get_cpu_number();    

    /** 得到当前调用栈
      * 注意事项: 编译源代码时带上-rdynamic和-g选项，否则可能看到的是函数地址，而不是函数符号名称
      * @call_stack: 存储调用栈
      * @return: 成功返回true，否则返回false
      */
    static bool get_backtrace(std::string& call_stack);

    /** 得到指定目录字节数大小，非线程安全函数，同一时刻只能被一个线程调用
      * @dirpath: 目录路径
      * @return: 目录字节数大小，如果为 -1  表示出错，可以通过 errno 得到出错原因
      */
    static off_t du(const char* dirpath);

    /** 得到内存页大小 */
    static int get_page_size();

    /** 得到一个进程可持有的最多文件(包括套接字等)句柄数 */
    static int get_fd_max();
    
    /** 下列is_xxx函数如果发生错误，则抛出CSyscallException异常 */
    static bool is_file(int fd);                 /** 判断指定fd对应的是否为文件 */
    static bool is_file(const char* path);       /** 判断指定Path是否为一个文件 */
    static bool is_link(int fd);                 /** 判断指定fd对应的是否为软链接 */
    static bool is_link(const char* path);       /** 判断指定Path是否为一个软链接 */
    static bool is_directory(int fd);            /** 判断指定fd对应的是否为目录 */
    static bool is_directory(const char* path);  /** 判断指定Path是否为一个目录 */
    
    /***
      * 是否允许当前进程生成coredump文件
      * @enable: 如果为true，则允许当前进程生成coredump文件，否则禁止
      * @core_file_size: 允许生成的coredump文件大小，如果取值小于0，则表示不限制文件大小
      * @exception: 如果调用出错，则抛出CSyscallException异常
      */
    static void enable_core_dump(bool enabled=true, int core_file_size=-1);

    /** 得到当前进程程序的名称，结果和main函数的argv[0]相同，如“./abc.exe”为“./abc.exe” */
    static std::string get_program_long_name();

    /**
     * 得到当前进程的的名称，不包含目录部分，如“./abc.exe”值为“abc.exe”
     * 如果调用了set_process_title()，
     * 则通过program_invocation_short_name可能取不到预期的值，甚至返回的是空
     */
    static std::string get_program_short_name();

    /**
     * 取路径的文件名部分，结果包含后缀部分，效果如下：
     *
     * path           dirpath        basename
     * "/usr/lib"     "/usr"         "lib"
     * "/usr/"        "/"            "usr"
     * "usr"          "."            "usr"
     * "/"            "/"            "/"
     * "."            "."            "."
     * ".."           "."            ".."
     */
    static std::string get_filename(const std::string& filepath);

    /** 取路径的目录部分，不包含文件名部分，并保证不以反斜杠结尾 */
    static std::string get_dirpath(const std::string& filepath);

    /***
      * 设置线程的名称，即“/proc/PID/comm”，对killall有效
      * @new_name 新的名字，通过top查看线程时（不是ps），可以看到线程的名字
      * @exception: 如果调用出错，则抛出CSyscallException异常
      */
    static void set_process_name(const std::string& new_name);
    static void set_process_name(const char* format, ...);

    /***
      * 设置进程标题，ps命令看到的结果（不是top），，对killall无效
      * 必须先调用init_program_title()后，才可以调用set_program_title()
      */
    static void init_process_title(int argc, char *argv[]);    
    static void set_process_title(const std::string& new_title);
    static void set_process_title(const char* format, ...);

    // 取随机数
    template <typename T>
    static T get_random_number(unsigned int i, T max_number)
    {
        struct timeval tv;
        struct timezone *tz = NULL;

        gettimeofday(&tv, tz);
        srandom(tv.tv_usec + i); // 加入i，以解决过快时tv_usec值相同

        // RAND_MAX 类似于INT_MAX
        return static_cast<T>(random() % max_number);
    }

    // 将一个vector随机化
    template <typename T>
    static void randomize_vector(std::vector<T>& vec)
    {
        unsigned int j = 0;
        std::vector<T> tmp;

        while (!vec.empty())
        {
            typename std::vector<T>::size_type max_size = vec.size();
            typename std::vector<T>::size_type random_number = mooon::sys::CUtils::get_random_number(j, max_size);

            typename std::vector<T>::iterator iter = vec.begin() + random_number;
            tmp.push_back(*iter);
            vec.erase(iter);
            ++j;
        }

        vec.swap(tmp);
    }

    // 取一个随机字符串
    static std::string get_random_string(const char* prefix=NULL);

    // 指定进程ID的进程是否存在
    static bool process_exists(int64_t pid);

    // 根据进程ID，得到进程名（为top看到的名称，非ps看到的名称，所以对killall有效）
    // 如果进程不存在，或没有权限，没返回空字符串
    static std::string get_process_name(int64_t pid);

    // 取得指定名称的所有进程ID（set_process_name设置的名字有效）
    // regex 是否正则匹配
    //
    // 出错返回-1，否则返回符合的个数
    static int get_all_pid(const std::string& process_name, std::vector<int64_t>* pid_array, bool regex=false);

    // 杀死指定名称的进程
    // process_name 需要杀死的进程名（set_process_name设置的名字有效）
    // 限制 除非为root，否则没有权限杀死其它用户的进程
    //
    // 返回被杀死的个数和出错的个数（包含不存在的），
    // 如果出错，则第一个值为-1，第二个值为系统出错代码（即errno值）
    static std::pair<int, int>
    killall(const std::string& process_name, int signo, bool regex=false);

    // 指定名称的进程是否存在
    static bool process_exists(const std::string& process_name, bool regex=false);

    // 杀死进程
    // 返回操作成功的个数
    static int killall(const std::vector<int64_t>& pid_array, int signo);

    // 全局唯一ID
    static std::string get_guid(const std::string& tag);
};

SYS_NAMESPACE_END
#endif // MOOON_SYS_UTILS_H
