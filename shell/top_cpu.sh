#!/bin/bash
# Writed by yijian on 2019/9/5
# 将CPU占用率Top10的进程记录到文件/tmp/cpu.top中，
# 带一个备份文件/tmp/cpu.top.bak
# 记录频率10秒，至少记录最近2小时数据
# 注意不要同时运行多个本脚本。
#
# 可选带两个参数：
# 参数1）记录频率，默认为10秒
# 参数2）记录top多少个，默认为10个

# 记录频率
# 单位：秒
loginterval=10
# 日志文件
topfile=/tmp/cpu.top
# 备份日志文件
topfileback=$topfile.bak
# 日志文件大小（默认2M）
topfilesize=2097152
# 记录top多少个
topN=10

# 带滚动的日志函数
# 带一个参数：需要记录的日志
function writelog()
{
  log="$1"
  logtime=`date +"%Y-%m-%d %H:%M:%S"`
  echo -en "====================\n[$logtime]\n$log\n\n" >> $topfile

  size=`ls -l --time-style=long-iso $topfile | awk '{print $5}'`
  if test ! -z "$size" -a $size -ge $topfilesize; then
    mv $topfile $topfileback
    echo -en "[$logtime]\n$log\n\n" > $topfile
  fi
}

# 如果至少有一个参数
if test $# -ge 1; then
  loginterval="$1"
fi
# 如果至少有一个参数
if test $# -ge 2; then
  topN="$2"
fi

# 定时记录，
# 间隔时长由LOG_INTERVAL值决定
set -e
while (true)
do
  log=`ps -Ao pcpu,pmem,rss,vsz,comm,pid,user,time,args --sort=-pcpu | head -n $topN`
  writelog "$log"
  sleep $loginterval
done
set +e
