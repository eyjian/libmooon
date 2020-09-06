#!/bin/sh
# Writed by yijian on 2020/9/6
# 查看指定进程名的 top 数据
# 执行依赖 top 和 awk 两个命令，
# 支持 mooon_ssh 远程批量执行。
#
# 带一个参数：
# 1）参数1：进程名

# 使用帮助函数
function usage()
{
  echo "Top then process with given name."
  echo "Usage: `basename $0` process-name"
  echo "Example: `basename $0` redis-server"
}

# 参数检查
if test $# -ne 1; then
  usage
  exit 1
fi

# 参数指定的 redis-server 监听端口
PROCESS_NAME="$1"

# top 命令可能位于不同目录下
TOP=/usr/bin/top
which $TOP > /dev/null 2>&1
if test $? -ne 0; then
  TOP=/bin/top
  which $TOP > /dev/null 2>&1
  if test $? -ne 0; then
    echo "\`top\` is not exists or is not executable."
    exit 1
  fi
fi

# 取得进程ID（可能多个）
# 命令 ps 输出的时间格式有两种：“7月17”和“20:42”，所以端口所在字段有区别：
PIDs=(`ps -f -C "$PROCESS_NAME" | awk -F'[ :]*' '{ print $2 }'`)
if test -z "$PIDs" -o ${#PIDs[@]} -eq 0; then
  echo "Can not get PIDs of \"$PROCESS_NAME\""
  exit 1
fi

# 执行 top 之前需要设置好环境变量“TERM”，否则执行将报如下错：
# TERM environment variable not set.
export TERM=xterm

for ((i=0; i<${#PIDs[@]}; ++i))
do
  pid=${PIDs[i]}
  # 过滤掉标题
  if test "$pid" = "PID"; then
    continue
  fi

  # 后台方式执行命令 top 时，
  # 需要加上参数“-b”（非交互模式），不然报错“top: failed tty get”
  #$TOP -b -p $pid -n 1 | grep redis-serv+
  eval $($TOP -b -p $pid -n 1 | awk '{ if (match($12,"redis-serv+")) printf("PID=%s\nUSER=%s\nPR=%s\nNI=%s\nVIRT=%s\nRES=%s\nSHR=%s\nS=%s\nCPU=%s\nMEM=%s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12); }')
  # PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
  echo -e "PID:\033[1;33m$PID\033[m USER:$USER PR:$PR NI:$NI VIRT:\033[1;33m$VIRT\033[m RES:\033[1;33m$RES\033[m SHR:$SHR S:$S %CPU:\033[1;33m$CPU\033[m %MEM:\033[1;33m$MEM\033[m"
done
