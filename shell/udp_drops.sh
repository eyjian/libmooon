#!/bin/bash
# Writed by yijian on 2018/9/3
# 统计UPD丢包工具
# 可选参数1：统计间隔（单位：秒，默认10秒）
# 可选参数2：是否输出丢包为0的记录，注意有参数1时，参数2才会生效
#
# 运行结果会写日志，日志文件优先存工具相同的目录，
# 但如果没有权限，则选择当前目录，
# 当前目录无权限，则存tmp目录，
# 如果tmp目录还无权限，则报错退出。
#
# 输出格式：统计日期 统计时间 丢包数
# 输出示例：
# 2018-09-03 17:22:49 5
# 2018-09-03 17:22:51 3

flag=0
stat_seconds=10
if test $# -gt 2; then
    echo "Usage: `basename $0` [seconds] [0|1]"
    exit 1
fi
if test $# -gt 1; then
    flag=$2 # 值为1表示输出丢包为0的记录
fi
if test $# -gt 0; then
    stat_seconds=$1    
fi

# 下段不允许出错
set -e

# 日志文件
basedir=$(dirname $(readlink -f $0))
logname=`basename $0 .sh`
logfile=$basedir/$logname.log
if test ! -w $basedir; then
    basedir=`pwd`
    logfile=$basedir/$logname
    
    if test ! -w $basedir; then
        basedir=/tmp
        logfile=$basedir/$logname
    fi
fi

# 备份日志文件
bak_logfile=$logfile.bak
if test -f $logfile; then
    rm --interactive=never $logfile
    touch $logfile
fi

# 恢复
set +e

# 统计哪些网卡，不填写则自动取
#ethX_array=() 
#
#if test $# -eq 0; then
#    ethX_array=(`cat /proc/net/dev| awk -F[\ \:]+ '/eth/{printf("%s\n",$2);}'`)
#else
#    ethX_array=($*)
#fi

old_num_errors=0
for ((;;))
do
    # 相关命令：
    # 1) 查看队列中的包数：netstat –alupt
    # 2) 查看socket读缓冲区大小：cat /proc/sys/net/core/rmem_default
    # 3) 查看socket读缓冲区大小：cat /proc/sys/net/core/wmem_default
    # 4) 查看网卡队列大小：ethtool -g eth1
    # 5) 查看arp缓存队列大小：cat /proc/sys/net/ipv4/neigh/eth1/unres_qlen
    # 6) 查看CPU负载：mpstat -P ALL 1 或 vmstat 1 或 top 或 htop 或uptime
    #
    # 取得丢包数
    # 命令“cat /proc/net/snmp | grep Udp”比命令“netstat –su”好
    # num_drops=`netstat -su | awk -F[\ ]+ 'BEGIN{flag=0;}{ if ($0=="Udp:") flag=1; if ((flag==1) && (match($0, "packet receive errors"))) printf("%s\n", $2); }'`
    num_errors=`cat /proc/net/snmp | awk -F'[ ]'+ 'BEGIN{ line=0; }/Udp/{ ++line; if (2==line) printf("%s\n", $4); }'`
    
    if test $old_num_errors -eq 0; then
        old_num_errors=$num_errors
    elif test $num_errors -ge $old_num_errors; then
        num_drops=$(($num_errors - $old_num_errors))

        if test $flag -eq 1 -o $num_drops -ne 0; then
            line="`date '+%Y-%m-%d %H:%M:%S'` $num_drops"

            # 得到日志文件大小（1073741824 = 1024 \* 1024 \* 1024）    
            logfile_size=`ls -l --time-style=long-iso $logfile 2>/dev/null| awk -F[\ ]+ '{ printf("%s\n", $5); }'`
            if test ! -z "$logfile_size"; then
                if test $logfile_size -gt 1073741824; then
                    echo $line | tee -a $logfile
                    mv $logfile $bak_logfile
                    rm -f $logfile
                fi
            fi

            echo $line | tee -a $logfile
        fi
    fi

    sleep $stat_seconds
done
