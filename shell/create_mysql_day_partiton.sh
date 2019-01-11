#!/bin/sh
# Writed by yijian on 2019/1/10
#
# 按天创建MySQL表RANGE分区工具：
# 运行一次可创建一年的分区，运行多次可为不同年份创建分区，
# 但注意：年份要从前往后，比如先2018再2019，依次类推。。。
# 如果是创建两年或以上年份的分区，则不同年份间的分区名，需通过最后一个参数来衔接
#
# 第一个参数指定4位数的年份，如2019，
# 本脚本会为该年份的每一天创建一个分区,
# 默认第一个分区名为p1，第二个为p2，依次类推。。。
# 但可以通过create的最后一个参数参数partition-start-index
# 指定待创建分区的起始序号，比如从2018开始建立分区：
# create_mysql_day_partiton.sh 2018 192.168.1.31 3306 root 123456 testdb testTable f_time create
# create_mysql_day_partiton.sh 2019 192.168.1.31 3306 root 123456 testdb testTable f_time create 366
# create_mysql_day_partiton.sh 2020 192.168.1.31 3306 root 123456 testdb testTable f_time create 731
# 通过最后一个参数，如：731，来确保分区名不会重复。
#
# 应用对象：
# 按年分表的表，即每年一张表，
# 这样一张表最多366个分区（闰年，平年则365个分区）
#
# 分区函数：TO_DAYS("2019-01-10 17:14:59")，不能用DAYOFYEAR等
# 注意：如果为一张表的多个年份创建分区，请按年份顺序
# 依次创建，并且必须指定参数partition-start-index，
# 否则会报分区重复错误：
# ERROR 1517 (HY000) at line 1: Duplicate partition name p2
#
# 在执行本脚本之前，
# 需先创建好目标表，但不要包含分区语句，
# 如不能包含“PARTITION BY”：
# DROP TABLE IF EXISTS `testTable`;
# CREATE TABLE `testTable`
# (
#     `f_id` BIGINT NOT NULL AUTO_INCREMENT,
#     `f_time` DATETIME NOT NULL,
#     `f_uid` VARCHAR(28) NOT NULL,
#     PRIMARY KEY (`f_time`,`f_uid`),
#     KEY (`f_id`),
#     KEY (`f_uid`)
# );
#
# 如果数据没有对应的分区，则INSERT时报错：
# ERROR 1526 (HY000): Table has no partition for value 737730

# 显示用法函数
function usage()
{
    # 创建分区（create表示创建分区）
    # 参数partition-start-index指定创建分区的起始序号，
    # 为可选参数，默认从1开始，即第一个创建的是p1，然后是p2，依次类推
    echo "Usage1: YEAR MySQL-ip MySQL-port MySQL-username MySQL-password MySQL-dbname MySQL-tablename partition-field create partition-start-index"
    # 仅仅测试（test表示仅测试输出创建分区语句，并不实际执行创建分区语句）
    echo "Usage2 (only test): YEAR MySQL-ip MySQL-port MySQL-username MySQL-password MySQL-dbname MySQL-tablename partition-field test partition-start-index"
    echo "Example1: create_mysql_day_partiton.sh 2018 192.168.1.31 3306 root 123456 testdb testTable f_time create"
    echo "Example2: create_mysql_day_partiton.sh 2019 192.168.1.31 3306 root 123456 testdb testTable f_time create 366"
    echo "Example3: create_mysql_day_partiton.sh 2018 192.168.1.31 3306 root 123456 testdb testTable f_time test"
    echo "Example4: create_mysql_day_partiton.sh 2019 192.168.1.31 3306 root 123456 testdb testTable f_time test 366"
}

# 依赖mysql
MYSQL=mysql
which "$MYSQL" > /dev/null 2>&1
if test $? -ne 0; then
    echo -e "\033[1;33m\`mysql\` not exists or not executable\033[m"
    exit 1
fi

# 需提供9个或10个参数
if test $# -ne 9 -a $# -ne 10; then
    usage
    exit 1
fi

year=$1                # 为哪一年创建分区
next_year=$(($year+1)) # 最后一就个分区用的是下一年一月一日
mysql_ip=$2            # MySQL的IP
mysql_port="$3"        # MySQL的端口号
mysql_username="$4"    # 连接MySQL的用户名
mysql_password="$5"    # 连接MySQL的密码
mysql_dbname="$6"      # MySQL的DB名
mysql_tablename="$7"   # 需要创建分区的表名
partition_field="$8"   # 用来分区的字段，字段类型应为DATETIME
only_test=0
if test "X$9" = "Xtest"; then
    only_test=1 # 不实际执行，仅仅测试生成创建分区语句
elif test "X$9" != "Xcreate"; then
    echo -e "\033[1;33mThe last parameter is invalid\033[m"
    usage
    exit 1
fi

# 判断是否为数字字符串
# 如果是数字字符串返回值为1，否则返回值为0
function is_number_str()
{
    str="$1"
    expr "$str" "+" "2019" &>/dev/null
    if test $? -eq 0; then
        echo "1"
    else
        echo "0"
    fi
}

#set -e
partition_start_index=0
if test $# -eq 10; then
    shift 9 # 超过9个参数了
    str="$1"
    n=`is_number_str "$str"`
    if test $n -ne 1; then
        echo -e "\033[1;33mParameter 'partition-start-index' is not a valid number: \`$str\`\033[m"
        usage
        exit 1
    fi
    partition_start_index="$str"
fi
if test $partition_start_index -lt 0; then
    echo -e "\033[1;33mParameter \`partition-start-index\` is less than 0\033[m"
    usage
    exit 1
fi
#set +e

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
echo "Partition start index: $partition_start_index"

# 得到指定月有多少天
function get_num_mdays()
{
    mdays=31 # 一月的天数
    month=$1 # 月份

    if test $month -eq 2; then
        if test $is_leap_year -eq 1; then
            mdays=29
        else
            mdays=28
        fi
    elif test $month -eq 4 -o $month -eq 6  -o $month -eq 9 -o $month -eq 11; then
        mdays=30
    fi

    echo $mdays
}

# 取得MySQL出错代码
function get_mysql_errcode()
{
    errcode=`echo "$1" | awk '{ print $2 }'`
    echo $errcode
}

months=12
partition_index=$partition_start_index
for ((i=1; i<=$months; ++i)) # 遍历一年的所有月
do
    mdays=`get_num_mdays $i`
    for ((j=1; j<=$mdays; ++j,++partition_index)) # 遍历一个月的所有天
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
        script="$MYSQL -h$mysql_ip -P$mysql_port -u$mysql_username -p'$mysql_password' $mysql_dbname \
-e\"ALTER TABLE $mysql_tablename ADD PARTITION(PARTITION p$partition_index VALUES LESS THAN (TO_DAYS('$datetime')))\""
        if test $i -eq 1 -a $j -lt 3; then # 一月二日为每一年的第一个分区
            if test $j -eq 1; then # 一月一日不需做分区
                continue
            fi
            
            # 如果不是测试，则执行创建分区
            echo "$script"
            if test $only_test -eq 0; then
                errmsg=`sh -c "$script" 2>&1`
                errcode=$?
                if test $errcode -eq 0; then
                    continue
                else
                    # 执行不成功
                    mysql_errcode=`get_mysql_errcode "$errmsg"`
                    echo -e "\033[1;33m$errmsg\033[m"

                    # ERROR 1505 (HY000) at line 1: Partition management on a not partitioned table is not possible
                    # ERROR 1517 (HY000) at line 1: Duplicate partition name p2
                    # ERROR 1493 (HY000) at line 1: VALUES LESS THAN value must be strictly increasing for each partition
                    # 如果是因为分区已存在，则可继续
                    if test "X$mysql_errcode" != "X1505"; then
                        exit 1
                    fi
                fi
            fi

            script="$MYSQL -h$mysql_ip -P$mysql_port -u$mysql_username -p'$mysql_password' $mysql_dbname \
-e\"ALTER TABLE $mysql_tablename PARTITION BY RANGE (TO_DAYS($partition_field)) (PARTITION p$partition_index VALUES LESS THAN (TO_DAYS('$datetime')))\""
        fi

        echo "$script"
        # 如果不是测试，则执行创建分区
        if test $only_test -eq 0; then
            errmsg=`sh -c "$script" 2>&1`
            errcode=$?
            if test $errcode -ne 0; then # 执行不成功
                echo -e "\033[1;33m$errmsg\033[m"
                exit 1
            fi
        fi
    done
done

script="$MYSQL -h$mysql_ip -P$mysql_port -u$mysql_username -p'$mysql_password' $mysql_dbname \
-e\"ALTER TABLE $mysql_tablename ADD PARTITION(PARTITION p$partition_index VALUES LESS THAN (TO_DAYS('$next_year-01-01 00:00:00')))\""
echo "$script"
# 如果不是测试，则执行创建分区
if test $only_test -eq 0; then
    sh -c "$script"
fi

# 正常退出
exit 0
