#!/bin/sh
# https://github.com/eyjian/libmooon
# Created by yijian on 2018/10/10
# 根据IP设置zookeeper的myid，
# 结合mooon_ssh和mooon_upload两个工具，可实现批量操作
#
# 参数1：
# IP和机器名映射关系配置文件
# 参数2：
# 全路径方式的远端myid文件
#
# 先按正常步骤部署好zookeeper，
# 在运行本工具时，会按照映射关系配置文件中定义的进行自动创建和设置myid，
# 结合mooon_ssh和mooon_upload避免了一台台机器低效修改。
#
# 使用方法：
# 1）利用mooon_upload，批量将本脚本文件发布到所有目标机器（需要修改hostname的机器）上
# 2）利用mooon_upload，批量将映射关系配置文件发布到所有目标机器上
# 3）利用mooon_ssh，批量执行本脚本完成主机名设置
#
# 使用示例：
# mooon_ssh -c='/tmp/set_zookeeper_id.sh /tmp/hosts.id /data/zookeeper/data/myid'

# 本机IP所在网卡
ethX=eth1

# 参数检查
if test $# -ne 2; then
    echo -e "\033[1;33musage\033[m:\n`basename $0` conffile myid"
    echo ""
    echo "conffile format:"
    echo "IP + separator + ID"
    echo ""
    echo "example:"
    echo "192.168.31.1 1"
    echo "192.168.31.2 2"
    echo "192.168.31.3 3"
    echo ""
    echo "available separators: space, TAB, semicolon, comma, vertical line"
    exit 1
fi

# 远端的Zookeeper的myid，需要包含全路径
myid=$2

# 检查配置文件是否存在和可读
conffile=$1
if test ! -f $conffile; then
    echo -e "file \`$conffile\` \033[0;32;31mnot exists\033[m"
    exit 1
fi
if test ! -r $conffile; then
    echo -e "file \`$conffile\` \033[0;32;31mno permission to read\033[m"
    exit 1
fi

# 取本机IP需要用到netstat
which netstat > /dev/null 2>&1
if test $? -ne 0; then
    echo "\`netstat\` command \033[0;32;31mnot available\033[m"
    exit 1
fi

# 需要有写权限
if test ! -w $kafkaconf; then
    echo "can not write $kafkaconf, or $kafkaconf not exits"
    exit 1
fi

# 取本地IP
local_ip=`netstat -ie|awk -F'[ :]+' -v ethX=$ethX 'BEGIN{ok=0;} {if (match($0, ethX)) ok=1; if ((1==ok) && match($0,"inet")) { ok=0; if (7==NF) printf("%s\n",$3); else printf("%s\n",$4); } }'`
if test -z "$local_ip"; then
    echo "can not get local IP of \`$ethX\`"
    exit 1
fi

# 读取文件，找到本机待设置的新主机名
while read line
do
    if test -z "$line"; then
        break
    fi

    eval $(echo $line |awk -F'[ \t|,;]+' '{ printf("ip=%s\nid=%s\n", $1, $2); }')
    #echo "[IP] => $ip  [ID] => $id"    
    if test "$ip" = "$local_ip"; then
        echo "$ip => $id"        
        echo "$id" > $myid
        exit 0
    fi
done < $conffile

echo "no item for $local_ip"
exit 1
