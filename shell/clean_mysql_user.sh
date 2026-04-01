#!/bin/sh
# Writed by yijian on 2023/08/16
# A tool to clean up mysql specified account.

if test $# -ne 5; then
    echo -e "Usage: \033[1;33m`basename $0`\033[m deleted_user mysql_host mysql_port mysql_root mysql_root_password"
    echo -e "Example: `basename $0` 'zhangsan' '127.0.0.1' 3306 'root' 'root123'"
    echo -e "\033[1;33mNOTICE\033[m: the tool will not delete the host which is '%'."
    exit 1
fi

deleted_user="$1" # 被清理的账号名
mysql_host="$2" # MySQL 的 host
mysql_port="$3" # MySQL 的 port
mysql_root="$4" # MySQL 的 root 账号名
mysql_root_password="$5" # MySQL 的 root 密码

set +e

deleted_count=0
list_sql="SELECT Host FROM mysql.user where User='$deleted_user'"
host_list=(`mysql -N --silent -h$mysql_host -P$mysql_port -u$mysql_root -p"$mysql_root_password" -e"$list_sql"`)

if test ${#host_list[@]} -eq 0; then
    echo -e "\033[0;32;31mNo any, exit now\033[m."
    exit 1
else
    echo -e "\033[1;33mList of hosts to delete for user '$deleted_user'\033[m:"
fi
for host in ${host_list[@]}; do
    if test "X$host" != "X%"; then
        delete_sql="DROP USER '$deleted_user'@'$host'"
        echo "$delete_sql"
    fi
done

echo -en "continue? [\033[1;33myes\033[m/\033[1;33mno\033[m]"
read -r -p " " choice
if test "$choice" != "yes"; then
    echo -e "\033[0;32;31mDo nothing, exit now\033[m."
    exit 1
fi

for host in ${host_list[@]}; do
    if test "X$host" != "X%"; then
        deleted_count=$((++deleted_count))
        delete_sql="DROP USER '$deleted_user'@'$host'"
        mysql -N --silent -h$mysql_host -P$mysql_port -u$mysql_root -p"$mysql_root_password" -e"$delete_sql"
    fi
done

if test $deleted_count -gt 0; then
    echo "$deleted_count deleted"
    mysql -N --silent -h$mysql_host -P$mysql_port -u$mysql_root -p"$mysql_root_password" -e"FLUSH PRIVILEGES"
fi

set -e

