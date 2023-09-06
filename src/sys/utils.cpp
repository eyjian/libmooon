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
#include "sys/utils.h"
#include "sys/atomic.h"
#include "sys/close_helper.h"
#include "sys/dir_utils.h"
#include "utils/md5_helper.h"
#include "utils/string_utils.h"

#if __cplusplus >= 201103L
    #include <chrono>
    #include <system_error>
    #include <thread>
#endif // __cplusplus >= 201103L

#ifdef __GNUC__
    #include <ext/stdio_filebuf.h>
#endif // __GNUC__

#include <arpa/inet.h>
#include <dirent.h>
#include <execinfo.h> // backtrace和backtrace_symbols函数
#include <features.h> // feature_test_macros
#include <ftw.h> // ftw
#include <libgen.h> // dirname&basename
#include <netdb.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <poll.h>
#include <pwd.h> // getpwuid
#include <regex.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/prctl.h> // prctl
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>

#ifndef PR_SET_NAME
#define PR_SET_NAME 15
#endif
 
#ifndef PR_GET_NAME
#define PR_GET_NAME 16
#endif

////////////////////////////////////////////////////////////////////////////////
SYS_NAMESPACE_BEGIN

// 和set_program_title相关
static char *g_arg_start = NULL;
static char *g_arg_end   = NULL;
static char *g_env_start = NULL;

pid_t gettid(void)
{
    return syscall(SYS_gettid);
}

int tgkill(int tgid, int tid, int sig)
{
    return syscall(SYS_tgkill, tgid, tid, sig);
}

// 取得 fd 对应的 inode
ino_t get_inode(int fd)
{
    struct stat st;
    return (fstat(fd, &st) == -1)? 0: st.st_ino;
}

ino_t get_inode(const char *path)
{
    struct stat st;
    return (stat(path, &st) == -1)? 0: st.st_ino;
}

#ifdef __GNUC__
    ino_t get_inode(const std::ifstream& ifs)
    {
        const int fd = ifstream2fd(ifs);
        return (fd == -1)? 0: get_inode(fd);
    }

    // __gnu_cxx::stdio_filebuf:
    // Provides a layer of compatibility for C/POSIX.

    // 使用 dynamic_cast，需要开启”-frtti“，而指定”-fno-rtti“可禁用，而且要求对象含虚表指针和有虚函数表

    // 仅 GNU C++ 库可用
    int ifstream2fd(const std::ifstream& fs)
    {
        // fs.rdbuf() 返回的为“std::basic_filebuf<char, std::char_traits<char> >*”类型
        //
        // auto rdbuf = fs.rdbuf();
        // (gdb) p *rdbuf
        // $8 = {<std::basic_streambuf<char, std::char_traits<char> >> = {_vptr.basic_streambuf = 0x7ffff7bbfc30 <vtable for std::basic_filebuf<char, std::char_traits<char> >+16>,
        //
        // std::ifstream 底层用的是 std::basic_filebuf，而不是 __gnu_cxx::stdio_filebuf，所以 dynamic_cast 不起作用。
        // 但是 stdio_filebuf 在 basic_filebuf 的基础上没有增加新的数据成员，除了自己的指向虚拟函数表的指针，使得可安全的将 basic_filebuf 强制转换为 stdio_filebuf 使用。
        // 实际上即使 stdio_filebuf 有新增数据成员，只要不使用到，也是没有问题的，也就是 stdio_filebuf 只能安全地使用 basic_filebuf 有的数据成员，其自身的在这种情况是未初始化的。

        // __gnu_cxx::stdio_filebuf<char>* filebuf = dynamic_cast<__gnu_cxx::stdio_filebuf<char>*>(fs.rdbuf());
        __gnu_cxx::stdio_filebuf<char>* filebuf = static_cast<__gnu_cxx::stdio_filebuf<char>*>(fs.rdbuf());
        return (filebuf != NULL)? filebuf->fd(): -1;
    }

    int ofstream2fd(const std::ifstream& fs)
    {
        //__gnu_cxx::stdio_filebuf<char>* filebuf = dynamic_cast<__gnu_cxx::stdio_filebuf<char>*>(fs.rdbuf());
        __gnu_cxx::stdio_filebuf<char>* filebuf = static_cast<__gnu_cxx::stdio_filebuf<char>*>(fs.rdbuf());
        return (filebuf != NULL)? filebuf->fd(): -1;
    }
#endif // __GNUC__

// 函数rdtsc取自Linux内核（linux-4.20）源代码，
// 所在文件：tools/perf/jvmti/jvmti_agent.c
//
// RDTSC: ReaD Time Stamp Counter
// 由于RDTSC指令跟CPU核相关，
// 因此使用时为得到准确数据，需要线程和CPU核心建立亲和关系
//
// 另一CPU指令CPUID可用于获取CPU的厂商信息
//
// 相关：
// HPET: 高精度定时器（High Precision Event Timer），依赖Linux内核的墙上时间xtime
uint64_t CUtils::rdtsc()
{
#if defined(__i386__) || defined(__x86_64__)
    unsigned int low, high;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));
    return low | ((uint64_t)high) << 32;
#else
    return 0;
#endif
}

// #include <chrono>
// #include <system_error>
// std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
// Compared  to sleep(3) and usleep(3), nanosleep() has the following advantages:
// it provides a higher resolution for specifying the sleep interval;
// POSIX.1 explicitly specifies that it does not interact with signals;
// and it makes the task of resuming a sleep that has been interrupted by a signal handler easier.
//
// If the call is interrupted by a signal handler, nanosleep() returns -1, sets errno to EINTR,
// and writes the remaining time into the structure pointed to by rem unless rem  is NULL.
// The value of *rem can then be used to call nanosleep() again and complete the specified pause (but see NOTES).
//
// nanosleep()  suspends  the execution of the calling thread until either
// at least the time specified in *req has elapsed,
// or the delivery of a signal that triggers the invocation of a handler in the calling thread or that terminates the process.
//
// // _POSIX_C_SOURCE >= 199309L
// int nanosleep(const struct timespec *req, struct timespec *rem);
// struct timespec {
//     time_t tv_sec;        /* seconds */
//     long   tv_nsec;       /* nanoseconds, value of the nanoseconds field must be in the range 0 to 999999999 */
// };
//
// int clock_nanosleep(...); // high-resolution sleep with specifiable clock
// int restart_syscall(void); // restart a system call after interruption by a stop signal
//
// POSIX.1-2001声明此函数已废弃，使用nanosleep代替，POSIX.1-2008删除了usleep
// int usleep(useconds_t usec); // suspends execution of the calling thread for (at least) usec microseconds.
// On the original BSD implementation, and in glibc before version 2.2.2, the return type of this function is void.
// The POSIX version returns int, and this is also  the  proto‐type used since glibc 2.2.2.
//
// sleep可能基于SIGALRM实现，一定不能和alarm混合使用！！！
// // makes the calling thread sleep until seconds seconds have elapsed or a signal arrives which is not ignored.
// // sleep() may be implemented using SIGALRM; mixing calls to alarm(2) and sleep() is a bad idea.
// // Zero if the requested time has elapsed, or the number of seconds left to sleep, if the call was interrupted by a signal handler.
// unsigned int sleep(unsigned int seconds);
void CUtils::millisleep(uint32_t milliseconds)
{
    struct timespec ts = { milliseconds / 1000, (milliseconds % 1000) * 1000000 };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));

    return;
#if __cplusplus >= 201103L
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#endif
}

// #include <chrono>
// #include <system_error>
//std::this_thread::sleep_for(std::chrono::microseconds(1000));
void CUtils::microsleep(uint32_t microseconds)
{
    struct timespec ts = { microseconds / 1000000, (microseconds % 1000000) * 1000 };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));

    return;
#if __cplusplus >= 201103L
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
#endif
}

void CUtils::pollsleep(int milliseconds)
{
    // 可配合libco协程库，但可能被中断提前结束而不足milliseconds
    // 如果被中断，则sleep时长将不足milliseconds
    //
    // Specifying a negative value in timeout means an infinite timeout.
    // Note that the timeout interval will be rounded up to the system clock granularity,
    // and kernel scheduling delays mean that the blocking interval may overrun by a small amount.
    // Specifying a timeout of zero causes poll() to return immediately.
    // On success, a positive number is returned,
    // onerror, -1 is returned, and errno is set appropriately.
    (void)poll(NULL, 0, milliseconds);
    // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
}

// struct timeval {
//     long    tv_sec;         /* seconds */
//     long    tv_usec;        /* microseconds */
// };
// int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
void CUtils::selectsleep(int milliseconds)
{
    // may update the timeout argument to indicate how much time was left,
    // pselect() does not change this argument.
    struct timeval timeout = { milliseconds / 1000, (milliseconds % 1000) };
    while (true)
    {
        (void)select(0, NULL, NULL, NULL, &timeout);
        // 如果被中断提前结束，则timeout存储了剩余未睡眠的时长
        if (timeout.tv_sec<=0 && timeout.tv_usec<=0)
            break;
    }
}

std::string CUtils::get_error_message(int errcode)
{
    return Error::to_string(errcode);
}

std::string CUtils::get_last_error_message()
{
    return Error::to_string();
}

int CUtils::get_last_error_code()
{
    return Error::code();
}

uint32_t CUtils::get_current_process_id()
{
    return static_cast<uint32_t>(getpid());
}

uint32_t CUtils::get_current_userid()
{
    return static_cast<uint32_t>(getuid());
}

std::string CUtils::get_current_username()
{
    char *buf;
    struct passwd pwd;
    struct passwd* result;
    std::string username;

    long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (-1 == bufsize)
    {
        bufsize = 16384;        /* Should be more than enough */
    }

    buf = new char[bufsize];
    const uint32_t uid = get_current_userid();
    getpwuid_r(uid, &pwd, buf, bufsize-1, &result);
    if (result != NULL)
    {
        username = pwd.pw_name;
    }

    delete []buf;
    return username;
}

std::string CUtils::get_program_fullpath()
{
    const size_t bufsize = PATH_MAX;
    char* buf = new char[bufsize];
    const int retval = readlink("/proc/self/exe", buf, bufsize-1);
    if (retval > 0)
    {
        buf[retval] = '\0';
    }
    else
    {
        *buf = '\0';
    }

    const std::string fullpath(buf);
    delete []buf;
    return fullpath;
}

std::string CUtils::get_program_path()
{
    return get_program_dirpath();
}

std::string CUtils::get_program_dirpath()
{
    const size_t bufsize = PATH_MAX;
    char* buf = new char[bufsize];
    const int n = readlink("/proc/self/exe", buf, bufsize-1);

    if (n > 0)
    {
        buf[n] = '\0';

#if 0 // 保留这段废代码，以牢记deleted的存在，但由于这里只取路径部分，所以不关心它的存在
        if (!strcmp(buf+retval-10," (deleted)"))
            buf[retval-10] = '\0';
#else

        // 去掉文件名部分
        char* end = strrchr(buf, '/');
        if (NULL == end)
            buf[0] = 0;
        else
            *end = '\0';
#endif
    }
    else
    {
        buf[0] = '\0';
    }

    const std::string dirpath = buf;
    delete []buf;
    return dirpath;
}

int CUtils::get_program_parameters(std::vector<std::string>* parameters, uint32_t pid)
{
    char* raw_full_cmdline_buf = new char[PATH_MAX];

    std::string cmdline_filepath;
    if (0 == pid)
        cmdline_filepath = "/proc/self/cmdline";
    else
        cmdline_filepath = utils::CStringUtils::format_string("/proc/%u/cmdline", pid);

    const int fd = open(cmdline_filepath.c_str(), O_RDONLY);
    if (fd != -1)
    {
        const int n = read(fd, raw_full_cmdline_buf, PATH_MAX-1);
        if (n != -1)
        {
            raw_full_cmdline_buf[n] = '\0';

            std::string parameter;
            for (int i=0; i<=n; ++i)
            {
                if (i == n)
                {
                    if (!parameter.empty())
                        parameters->push_back(parameter);
                    break;
                }
                if ('\0' == raw_full_cmdline_buf[i])
                {
                    if (!parameter.empty())
                    {
                        parameters->push_back(parameter);
                        parameter.clear();
                    }
                }
                else
                {
                    parameter.push_back(raw_full_cmdline_buf[i]);
                }
            }
        }

        close(fd);
    }

    delete []raw_full_cmdline_buf;
    return static_cast<int>(parameters->size());
}

std::string CUtils::get_program_parameter0(uint32_t pid)
{
    std::string parameter0;
    std::vector<std::string> parameters;
    get_program_parameters(&parameters, pid);
    if (!parameters.empty())
        parameter0 = parameters[0];
    return parameter0;
}

std::string CUtils::get_program_full_cmdline(char separator, uint32_t pid)
{
    std::string full_cmdline;
    std::vector<std::string> parameters;

    const int n = get_program_parameters(&parameters, pid);
    for (int i=0; i<n; ++i)
    {
        if (full_cmdline.empty())
        {
            full_cmdline = parameters[i];
        }
        else
        {
            full_cmdline.push_back(separator);
            full_cmdline.append(parameters[i]);
        }
    }

    return full_cmdline;
}

std::string CUtils::get_filename(int fd)
{
    std::string path_buf(PATH_MAX+1, '\0');
    std::string filepath_buf(FILENAME_MAX+1, '\0');

    snprintf(const_cast<char*>(path_buf.data()), PATH_MAX, "/proc/%d/fd/%d", getpid(), fd);
    const ssize_t n = readlink(path_buf.c_str(), const_cast<char*>(filepath_buf.data()), FILENAME_MAX);
    if (n == -1)
        filepath_buf.clear();
    else
        filepath_buf.resize(n);
    return filepath_buf;
}

// 库函数：char *realpath(const char *path, char *resolved_path);
//         char *canonicalize_file_name(const char *path);
std::string CUtils::get_full_directory(const char* directory)
{
    std::string full_directory;
    DIR* dir = opendir(directory);
    if (dir != NULL)
    {
        int fd = dirfd(dir);
        if (fd != -1)
            full_directory = get_filename(fd);

        closedir(dir);
    }
 
    return full_directory;
}

// 相关函数：
// get_nprocs()，声明在sys/sysinfo.h
// sysconf(_SC_NPROCESSORS_CONF)
// sysconf(_SC_NPROCESSORS_ONLN)
uint16_t CUtils::get_cpu_number()
{
	FILE* fp = fopen("/proc/cpuinfo", "r");
	if (NULL == fp) return 1;
	
	char line[LINE_MAX];
	uint16_t cpu_number = 0;
    sys::CloseHelper<FILE*> ch(fp);

	while (fgets(line, sizeof(line)-1, fp))
	{
		char* name = line;
		char* value = strchr(line, ':');
		
		if (NULL == value)
			continue;

		*value++ = 0;		
		if (0 == strncmp("processor", name, sizeof("processor")-1))
		{
			 if (!utils::CStringUtils::string2uint16(value, cpu_number))
             {
                 return 0;
             }
		}
	}

	return (cpu_number+1);
}

bool CUtils::get_backtrace(std::string& call_stack)
{
    const int frame_number_max = 20;       // 最大帧层数
    void* address_array[frame_number_max]; // 帧地址数组

    // real_frame_number的值不会超过frame_number_max，如果它等于frame_number_max，则表示顶层帧被截断了
    int real_frame_number = backtrace(address_array, frame_number_max);

    char** symbols_strings = backtrace_symbols(address_array, real_frame_number);
    if (NULL == symbols_strings)
    {
        return false;
    }
    else if (real_frame_number < 2) 
    {
        free(symbols_strings);
        return false;
    }

    call_stack = symbols_strings[1]; // symbols_strings[0]为get_backtrace自己，不显示
    for (int i=2; i<real_frame_number; ++i)
    {
        call_stack += std::string("\n") + symbols_strings[i];
    }

    free(symbols_strings);
    return true;
}

static __thread off_t dirsize; // 目录大小
static int _du_fn(const char *fpath, const struct stat *sb, int typeflag)
{   
    if (FTW_F == typeflag)
        dirsize += sb->st_size;
    return 0;
}

// 获取指定目录大小函数（线程安全，但仅适用Linux）
// 遍历方式实现，低性能
off_t CUtils::du(const char* dirpath)
{
    dirsize = 0;
    return (ftw(dirpath, _du_fn, 0) != 0)? -1: dirsize;
}

int CUtils::get_page_size()
{
    // sysconf(_SC_PAGE_SIZE);
    // sysconf(_SC_PAGESIZE);
    return getpagesize();
}

int CUtils::get_fd_max()
{
    // sysconf(_SC_OPEN_MAX);
    return getdtablesize();
}

bool CUtils::is_file(int fd)
{
    struct stat buf;
    if (-1 == fstat(fd, &buf))
        THROW_SYSCALL_EXCEPTION(NULL, errno, "fstat");

    return S_ISREG(buf.st_mode);
}

bool CUtils::is_file(const char* path)
{
    struct stat buf;
    if (-1 == stat(path, &buf))
        THROW_SYSCALL_EXCEPTION(NULL, errno, "stat");

    return S_ISREG(buf.st_mode);
}

bool CUtils::is_link(int fd)
{
    struct stat buf;
    if (-1 == fstat(fd, &buf))
        THROW_SYSCALL_EXCEPTION(NULL, errno, "fstat");

    return S_ISLNK(buf.st_mode);
}

bool CUtils::is_link(const char* path)
{
    struct stat buf;
    if (-1 == stat(path, &buf))
        THROW_SYSCALL_EXCEPTION(NULL, errno, "stat");

    return S_ISLNK(buf.st_mode);
}

bool CUtils::is_directory(int fd)
{
    struct stat buf;
    if (-1 == fstat(fd, &buf))
        THROW_SYSCALL_EXCEPTION(NULL, errno, "fstat");

    return S_ISDIR(buf.st_mode);
}

bool CUtils::is_directory(const char* path)
{
    struct stat buf;
    if (-1 == stat(path, &buf))
        THROW_SYSCALL_EXCEPTION(NULL, errno, "stat");

    return S_ISDIR(buf.st_mode);
}

void CUtils::enable_core_dump(bool enabled, int core_file_size)
{    
    if (enabled)
    {
        struct rlimit rlim;
        rlim.rlim_cur = (core_file_size < 0)? RLIM_INFINITY: core_file_size;
        rlim.rlim_max = rlim.rlim_cur;

        if (-1 == setrlimit(RLIMIT_CORE, &rlim))
            THROW_SYSCALL_EXCEPTION(NULL, errno, "setrlimit");
    }       
    
    if (-1 == prctl(PR_SET_DUMPABLE, enabled? 1: 0))
        THROW_SYSCALL_EXCEPTION(NULL, errno, "prctl");
}

std::string CUtils::get_program_long_name()
{
    //#define _GNU_SOURCE
    //#include <errno.h>
    return program_invocation_name;
}

// 如果调用了set_process_title()，
// 则通过program_invocation_short_name可能取不到预期的值，甚至返回的是空
std::string CUtils::get_program_short_name()
{
    //#define _GNU_SOURCE
    //#include <errno.h>
    if ('/' == program_invocation_short_name[0])
        return program_invocation_short_name+1;
    else
        return program_invocation_short_name;
}

std::string CUtils::get_filename(const std::string& filepath)
{
    // basename的参数即是输入，也是输出参数，所以需要tmp_filepath
    std::string tmp_filepath(filepath);
    return basename(const_cast<char*>(tmp_filepath.c_str())); // #include <libgen.h>
}

std::string CUtils::get_dirpath(const std::string& filepath)
{
    // basename的参数即是输入，也是输出参数，所以需要tmp_filepath
    std::string tmp_filepath(filepath);
    return dirname(const_cast<char*>(tmp_filepath.c_str())); // #include <libgen.h>
}

void CUtils::set_process_name(const std::string& new_name)
{
    if (!new_name.empty())
    {
        // #include <libiberty.h> // libiberty.a
        // Set the title of a process
        // extern void setproctitle (const char *name, ...);
        // setproctitle也是对top有效，对ps无效
        if (-1 == prctl(PR_SET_NAME, new_name.c_str()))
            THROW_SYSCALL_EXCEPTION(NULL, errno, "prctl");
    }
}

void CUtils::set_process_name(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    utils::VaListHelper vlh(args);

    char name[NAME_MAX];
    vsnprintf(name, sizeof(name), format, args);
    CUtils::set_process_name(std::string(name));
}

// 参考：http://www.stamhe.com/?p=416
void CUtils::init_process_title(int argc, char *argv[])
{
    g_arg_start = argv[0];
    g_arg_end = argv[argc-1] + strlen(argv[argc-1]) + 1;
    g_env_start = environ[0];
}

void CUtils::set_process_title(const std::string &new_title)
{
    MOOON_ASSERT(g_arg_start != NULL);
    MOOON_ASSERT(g_arg_end != NULL);
    MOOON_ASSERT(g_env_start != NULL);

    if ((g_arg_start != NULL) && (g_arg_end != NULL) && (g_env_start != NULL) && !new_title.empty())
    {
        size_t new_title_len = new_title.length();

        // 新的title比老的长
        if ((static_cast<size_t>(g_arg_end-g_arg_start) < new_title_len)
         && (g_env_start == g_arg_end))
        {
            char *env_end = g_env_start;
            for (int i=0; environ[i]; ++i)
            {
                if (env_end != environ[i])
                {
                    break;
                }

                env_end = environ[i] + strlen(environ[i]) + 1;
                environ[i] = strdup(environ[i]);
            }

            g_arg_end = env_end;
            g_env_start = NULL;
        }

        size_t len = g_arg_end - g_arg_start;
        if (len == new_title_len)
        {
             strcpy(g_arg_start, new_title.c_str());
        }
        else if(new_title_len < len)
        {
            strcpy(g_arg_start, new_title.c_str());
            memset(g_arg_start+new_title_len, 0, len-new_title_len);

#if 0 // 从实际的测试来看，如果开启以下两句，会出现ps u输出的COMMAND一列为空
            // 当新的title比原title短时，
            // 填充argv[0]字段时，改为填充argv[0]区的后段，前段填充0
            memset(g_arg_start, 0, len);
            strcpy(g_arg_start+(len - new_title_len), new_title.c_str());
#endif
        }
        else
        {
            *(char *)mempcpy(g_arg_start, new_title.c_str(), len-1) = '\0';
        }

        if (g_env_start != NULL)
        {
            char *p = strchr(g_arg_start, ' ');
            if (p != NULL)
            {
                *p = '\0';
            }
        }
    }
}

void CUtils::set_process_title(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    utils::VaListHelper vlh(args);

    char title[PATH_MAX];
    vsnprintf(title, sizeof(title), format, args);
    CUtils::set_process_title(std::string(title));
}

std::string CUtils::get_random_string(const char* prefix)
{
    // 随机数因子
    static atomic_t s_random_factor = ATOMIC_INIT(0);
    const uint32_t random_factor = static_cast<uint32_t>(atomic_read(&s_random_factor));

    // 取得当前时间
    struct timeval tv;
    struct timezone *tz = NULL;
    gettimeofday(&tv, tz);

    // 随机数1
    srandom(tv.tv_usec + random_factor + 0);
    uint64_t m1 = random() % std::numeric_limits<uint64_t>::max();

    // 随机数2
    srandom(tv.tv_usec + random_factor + 1);
    uint64_t m2 = random() % std::numeric_limits<uint64_t>::max();

    // 随机数3
    srandom(tv.tv_usec + random_factor + 2);
    uint64_t m3 = random() % std::numeric_limits<uint64_t>::max();

    atomic_add(3, &s_random_factor);
    if (prefix == NULL)
        return utils::CStringUtils::format_string("%" PRIu64"%" PRIu64"%u%" PRIu64"%" PRIu64"%" PRIu64, static_cast<uint64_t>(tv.tv_sec), static_cast<uint64_t>(tv.tv_usec), random_factor, m1, m2, m3);
    else
        return utils::CStringUtils::format_string("%s%" PRIu64"%" PRIu64"%u%" PRIu64"%" PRIu64"%" PRIu64, prefix, static_cast<uint64_t>(tv.tv_sec), static_cast<uint64_t>(tv.tv_usec), random_factor, m1, m2, m3);
}

bool CUtils::process_exists(int64_t pid)
{
    return 0 == kill(pid, 0);
}

// int asprintf(char **strp, const char *fmt, ...);
std::string CUtils::get_process_name(int64_t pid)
{
    // 一些Linux并没有comm，因此取不到值
    const std::string& filepath = utils::CStringUtils::format_string("/proc/%" PRId64"/comm", pid);
    const int fd = open(filepath.c_str(), O_RDONLY);
    char process_name[PATH_MAX];

    if (-1 == fd)
    {
        process_name[0] = '\0';
    }
    else
    {

        const int n = read(fd, process_name, sizeof(process_name)-1);
        if (n > 0)
            process_name[n] = '\0';
        else
            process_name[0] = '\0';
        if ('\n' == process_name[n-1])
            process_name[n-1] = '\0'; // 删除结尾的“\n”
        close(fd);
    }

    return process_name;
}

int CUtils::get_all_pid(const std::string& process_name, std::vector<int64_t>* pid_array, bool regex)
{
    regex_t preg;

    if (regex)
    {
        if (regcomp(&preg, process_name.c_str(), REG_EXTENDED) != 0)
        {
            return -1;
        }
    }
    try
    {
        std::vector<std::string> subdir_names; // pid数组
        CDirUtils::list("/proc", &subdir_names, NULL, NULL);

        for (std::vector<std::string>::size_type i=0; i<subdir_names.size(); ++i)
        {
            const std::string& pid_str = subdir_names[i];
            uint32_t pid = 0;

            if (utils::CStringUtils::string2int(pid_str.c_str(), pid))
            {
                const std::string& process_name_ = get_process_name(pid);

                if (!regex)
                {
                    if (process_name_ == process_name)
                        pid_array->push_back(pid);
                }
                else
                {
                    regmatch_t pmatch[1]; // 存放匹配结果
                    if (0 == regexec(&preg, process_name_.c_str(), 1, pmatch, 0))
                        pid_array->push_back(pid);
                }
            }
        }

        if (regex)
            regfree(&preg);
        return static_cast<int>(pid_array->size());
    }
    catch (sys::CSyscallException& ex)
    {
        if (regex)
            regfree(&preg);
        return -1;
    }
}

std::pair<int, int>
CUtils::killall(const std::string& process_name, int signo, bool regex)
{
    std::pair<int, int> ret;
    std::vector<int64_t> pid_array;
    const int n = get_all_pid(process_name, & pid_array, regex);

    for (int i=0; i<n; ++i)
    {
        const int64_t pid = pid_array[i];

        if (pid > 0)
        {
            if (0 == kill(pid, signo))
                ++ret.first;
            else
                ++ret.second;
        }
    }

    return ret;
}

bool CUtils::process_exists(const std::string& process_name, bool regex)
{
    std::vector<int64_t> pid_array;
    return get_all_pid(process_name, &pid_array, regex) > 0;
}

int CUtils::killall(const std::vector<int64_t>& pid_array, int signo)
{
    int success = 0;

    for (std::vector<int64_t>::size_type i=0; i<pid_array.size(); ++i)
    {
        const pid_t pid = static_cast<pid_t>(pid_array[i]);

        if (pid > 0)
        {
            if (0 == kill(pid, signo))
                ++success;
        }
    }

    return success;
}

std::string CUtils::get_guid(const std::string& tag)
{
    utils::CMd5Helper md5;
    struct timeval tv;

    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd != -1)
    {
        sys::CloseHelper<int> ch(fd);
        struct ifconf ifc;
        struct ifreq ifr[10]; // 最多10个IP

        if (ioctl(fd, SIOCGIFCONF, (char*)&ifc) != -1)
        {
            for (size_t i=0; i<ifc.ifc_len/sizeof(struct ifreq); ++i)
            {
                // 获取指定网卡上的IP地址
                if (-1 == ioctl(fd, SIOCGIFADDR, (char *)&ifr[i])) continue; // getifaddrs/freeifaddrs
                if (NULL == ifr[i].ifr_name) continue;
                if (strcasecmp(ifr[i].ifr_name, "eth0") == 0 ||
                    strcasecmp(ifr[i].ifr_name, "eth1") == 0 ||
                    strcasecmp(ifr[i].ifr_name, "eth2") == 0)
                {
                    md5.update(ifr[i].ifr_hwaddr.sa_data, sizeof(ifr[i].ifr_hwaddr.sa_data));
                    break;
                }
            }
        }
    }

    gettimeofday(&tv, NULL);
    md5.update(reinterpret_cast<const char*>(&tv), sizeof(tv));
    srandom(tv.tv_sec);
    const long n1 = random();
    const long n2 = random();
    const long n3 = random();
    const long n4 = random();
    const long n5 = random();
    md5.update(reinterpret_cast<const char*>(&n1), sizeof(n1));
    md5.update(reinterpret_cast<const char*>(&n2), sizeof(n2));
    md5.update(reinterpret_cast<const char*>(&n3), sizeof(n3));
    md5.update(reinterpret_cast<const char*>(&n4), sizeof(n4));
    md5.update(reinterpret_cast<const char*>(&n5), sizeof(n5));
    if (!tag.empty())
        md5.update(tag);

    return md5.to_string();
}

SYS_NAMESPACE_END
