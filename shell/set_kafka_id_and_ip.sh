#!/bin/sh
# https://github.com/eyjian/mooon
# Created by yijian on 2018/8/10
# 根据IP设置kafka ID和IP的工具，
# 结合mooon_ssh和mooon_upload两个工具，可实现批量操作
#
# 参数1：
# IP和机器名映射关系配置文件
# 参数2：
# 远端的Kafka的server.properties，需要包含全路径
#
# 先按正常步骤部署好kafka，
# 因为每个节点的server.properties中的broker.id和listeners值不同，
# 需要先将两者的值分别设置为KAFKAID和KAFKAIP，
# 在运行本工具时，会按照映射关系配置文件中定义的进行自动替换，
# 结合mooon_ssh和mooon_upload避免了一台台机器低效修改。
#
# 注意：
# 1）server.properties中的“broker.id=1”一行要写成：broker.id=KAFKAID
# 下面两个是可选的，绑定指定的IP才需要：
# 2）“listeners=PLAINTEXT://:9092”一行写成：listeners=PLAINTEXT://KAFKAIP:9092
# 3）“advertised.listeners=PLAINTEXT://your.host.name:9092”一行写成：advertised.listeners=PLAINTEXT://KAFKAIP:9092
# 4）“host.name=local”一行写成：host.name=KAFKAIP
# 运行本工具时，会自动替换KAFKAID和KAFKAIP，如果没有指定“advertised.listeners”的值，则使用“listeners”的值。
#
# IP和ID映射关系配置文件格式：
# IP+分隔符+ID
# 示例：
# 192.168.31.32 32
# 192.168.31.33 33
# 分隔符可为空格、TAB、逗号、分号和竖线这几种
#
# 使用方法：
# 1）利用mooon_upload，批量将本脚本文件发布到所有目标机器（需要修改hostname的机器）上
# 2）利用mooon_upload，批量将映射关系配置文件发布到所有目标机器上
# 3）利用mooon_ssh，批量执行本脚本完成主机名设置
#
# 操作示例：
# export H=192.168.0.21,192.168.0.22,192.168.0.23,192.168.0.24,192.168.0.25
# export U=kafka
# export P='kafka^12345'
# mooon_upload -s=hosts.id,server.properties,set_kafka_id_and_ip.sh -d=/tmp
# mooon_ssh -c='/tmp/set_kafka_id_and_ip.sh /tmp/hosts.id /tmp/server.properties'
#
# cat hosts.id
# 192.168.0.21 21
# 192.168.0.22 22
# 192.168.0.23 23
# 192.168.0.24 24
# 192.168.0.25 25
#
# mooon_upload -s=hosts.id,set_kafka_id_and_ip.sh -d=/tmp
# mooon_ssh -c='/tmp/set_kafka_id_and_ip.sh /tmp/host.names
# mooon_ssh -c='cat /etc/hostname'

# 本机IP所在网卡
ethX=eth1

# 参数检查
if test $# -ne 2; then
    echo -e "\033[1;33musage\033[m:\n`basename $0` conffile kafkaconf"
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

# 远端的Kafka的server.properties，需要包含全路径
kafkaconf=$2

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
        # 如下错误：
        # sed: -e expression #1, char 17: unknown command: `K'
        # 表示分号后可能遗漏了“s|”
        sed -i "s|KAFKAID|$id|g;s|KAFKAIP|$ip|g" $kafkaconf        
        exit 0
    fi
done < $conffile

echo "no item for $local_ip"
exit 1
