#!/bin/sh
# Writed by yijian on 2019/1/10
#
# 创建MySQL表分区工具：
# 第一个参数指定4位数的年份，如2019，
# 本脚本会为该年份的每一天创建一个分区。
#
# 应用对象：
# 按年分表的表，即每年一张表，
# 这样一张表最多366个分区（闰年，平年则365个分区）
#
# 分区函数：DAYOFYEAR("2019-01-10 17:14:59")
# 约束：表不能跨年，要求按年分表（原因：防止单张表分区数太多）
#
# 在执行本脚本之前，
# 需先创建好目标表，但不要包含分区语句，
# 如不能包含“PARTITION BY”：
# DROP TABLE IF EXISTS `testTable_2019`;
# CREATE TABLE testTable_2019
# (
#     `f_id` BIGINT NOT NULL AUTO_INCREMENT,
#     `f_time` DATETIME NOT NULL,
#     `f_uid` VARCHAR(28) NOT NULL,
#     PRIMARY KEY (`f_time`,`f_uid`),
#     KEY (`f_id`),
#     KEY (`f_uid`)
# );
# 建议“表名”带上年份后缀或前、中缀
#
# 如果数据没有对应的分区，则INSERT时报错：
# ERROR 1526 (HY000): Table has no partition for value 737730

# 显示用法函数
function usage()
{
    # 创建分区（create表示创建分区）
    echo "Usage1: YEAR MySQL-ip MySQL-port MySQL-username MySQL-password MySQL-dbname MySQL-tablename partition-field create"
    # 仅仅测试（test表示仅测试输出创建分区语句，并不实际执行创建分区语句）
    echo "Usage2 (only test): YEAR MySQL-ip MySQL-port MySQL-username MySQL-password MySQL-dbname MySQL-tablename partition-field test"
}

# 依赖mysql
MYSQL=mysql
which "$MYSQL" > /dev/null 2>&1
if test $? -ne 0; then
    echo "\`mysql\` not exists or not executable"
    exit 1
fi

# 需提供9个参数
if test $# -ne 9; then
    usage
    exit 1
fi

year=$1             # 为哪一年创建分区
next_year=$(($year+1))
mysql_ip=$2         # MySQL的IP
mysql_port=$3       # MySQL的端口号
mysql_username=$4   # 连接MySQL的用户名
mysql_password="$5" # 连接MySQL的密码
mysql_dbname=$6     # MySQL的DB名
mysql_tablename=$7  # 需要创建分区的表名
partition_field=$8  # 用来分区的字段，字段类型应为DATETIME
only_test=0
if test "X$9" = "Xtest"; then
    only_test=1 # 不实际执行，仅仅测试生成创建分区语句
elif test "X$9" != "Xcreate"; then
    usage
    exit 1
fi

# 是否为闰年（为1表示闰年，为0表示平年）
is_leap_year=0
a=$(($year % 4))
b=$(($year % 100))
c=$(($year % 400))
# 闰年条件：
# ((0 == year%4) && (year%100 != 0)) || (0 == year%400)
if test $a -eq 0 -a $b -eq 0; then
    is_leap_year=1
elif test $c -eq 0; then
    is_leap_year=1
fi

# 一年的天数
num_dayofyear=365
leap_year="FALSE"
if test $is_leap_year -eq 1; then
    num_dayofyear=366 # 闰年比平年多一天
    leap_year="TRUE"
fi

# 打印输入信息
echo "YEAR: $year"
echo "IS LEAP YEAR: $leap_year"
echo "NUMBER OF DAYS: $num_dayofyear"
echo "MySQL: $mysql_ip:$mysql_port#$mysql_dbname:$mysql_tablename"

# 得到指定月有多少天
function get_num_days()
{
    days=31
    month=$1

    if test $month -eq 2; then
        if test $is_leap_year -eq 1; then
            days=29
        else
            days=28
        fi
    elif test $month -eq 4 -o $month -eq 6  -o $month -eq 9 -o $month -eq 11; then
        days=30
    fi

    echo $days
}

set -e
months=12
k=0
for ((i=1; i<=$months; ++i)) # 遍历一年的所有月
do
    days=`get_num_days $i`
    for ((j=1; j<=$days; ++j,++k)) # 遍历一个月的所有天
    do
        month=$i
        day=$j
        if test $i -lt 10; then
            month="0$i"
        fi
        if test $j -lt 10; then
            day="0$j"
        fi

        datetime="$year-$month-$day 00:00:00"
        if test $i -eq 1 -a $j -lt 3; then
            if test $j -eq 1; then
                continue;
            fi
            script="$MYSQL -h$mysql_ip -P$mysql_port -u$mysql_username -p'$mysql_password' $mysql_dbname \
-e\"ALTER TABLE $mysql_tablename PARTITION BY RANGE (TO_DAYS($partition_field)) (PARTITION p$k VALUES LESS THAN (TO_DAYS('$datetime')))\""
        else
            script="$MYSQL -h$mysql_ip -P$mysql_port -u$mysql_username -p'$mysql_password' $mysql_dbname \
-e\"ALTER TABLE $mysql_tablename ADD PARTITION(PARTITION p$k VALUES LESS THAN (TO_DAYS('$datetime')))\""
        fi

        echo "$script"
        if test $only_test -eq 0; then
            sh -c "$script"
        fi
    done
done

script="$MYSQL -h$mysql_ip -P$mysql_port -u$mysql_username -p'$mysql_password' $mysql_dbname \
-e\"ALTER TABLE $mysql_tablename ADD PARTITION(PARTITION p$k VALUES LESS THAN (TO_DAYS('$next_year-01-01 00:00:00')))\""
echo "$script"
if test $only_test -eq 0; then
    sh -c "$script"
fi
set +e

# 正常退出
exit 0
