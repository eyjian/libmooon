#!/bin/sh
# writed by yijian on 2019/8/20
# filename cpumem.sh
# 统计指定进程ID的CPU和内存数据工具
# 运行时可指定两个参数：
# 1）参数1：进程ID（必指定）
# 2）参数2：统计间隔（单位为秒，可选，如果不指定则默认为2秒）
#
# 运行时输出5列内容：
# [统计日期时间]CPU百分比,内存百分比,物理内存,虚拟内存

# 显示用法函数
function usage()
{
    echo "Usage: cpumem.sh PID <interval>"
    echo "Example1: cpumem.sh 2019"
    echo "Example2: cpumem.sh 2019 1"
    echo "The interval parameter specifies the amount of time in seconds between each report."
}

# 检查参数个数
if test $# -lt 1 -o $# -gt 2; then
    usage
    exit 1
fi

pid=$1
interval=2
if test $# -eq 2; then
    interval=$2
fi
printf "[StatTime] THREADS %%CPU %%MEM RSS VSZ\n"
while (true)
do
    rss=0
    vsz=0
    stattime="`date +'%Y-%m-%d %H:%M:%S'`"

    # 取得进程的线程数
    threads=`awk -F[\ :]. '{if (match($1,"Threads")) print $2}' /proc/$pid/status`

    # rss和vsz的单位均为kb
    eval $(ps h -p $pid -o pcpu,pmem,rss,vsz | awk '{printf("cpu=%s\nmem=%s\nrss=%s\nvsz=%s\n",$1,$2,$3,$4);}')
    if test $rss -eq 0; then
        # 可能是进程已不存在
        kill -0 $PID > /dev/null 2>&1
        if test $? -ne 0; then
            echo "Process($PID) is not exists."
            break
        fi
    fi

    rssize="${rss}KB"
    vsize="${vsz}KB"
    if test $rss -ge 1048576; then
        # 大小达到GB
        rssize="`echo "scale=2;$rss/1024/1024" | bc -l`GB"
    elif test $rss -ge 1024; then
        # 大小达到MB
        rssize="`echo "scale=2;$rss/1024" | bc -l`MB"
    fi
    if test $vsz -ge 1048576; then
        # 大小达到GB
        vsize="`echo "scale=2;$vsz/1024/1024" | bc -l`GB"
    elif test $vsz -ge 1024; then
        # 大小达到MB
        vsize="`echo "scale=2;$vsz/1024" | bc -l`MB"
    fi

    # \033[1;33m 黄色
    # \033[1;32m 亮绿色
    printf "[%s] \033[0;32;31m%s\033[m \033[1;33m%s\033[m %s \033[1;32m%s\033[m %s\n" "$stattime" "$threads" "$cpu" "$mem" "$rssize" "$vsize";
    sleep $interval
done
