#!/bin/sh
# Writed by yijian on 2023/7/22
# 一键编译和安装 gcc 脚本，安装目录为 /usr/local
#
# 要求：
# 1、需以 root 权限执行本脚本
# 2、机器能够访问外网
# 3、机器上有命令：wget、gunzip、bunzip2、cmake
#
# 安装环境：X86_64，安装后不支持 32 位应用（--disable-multilib）
#
# 如果 make 时遇到错误“internal compiler error”，可能是因为内存不足，请换台内存更大的机器，或者更换GCC版本试试。

# 对 GCC 13.1.0，以下搭配不可以（Building GCC requires GMP 4.2+, MPFR 3.1.0+ and MPC 0.8.0+）：
# 1）gmp-6.2.0, mpfr-4.2.0, mpc-1.3.0 # mpc 版本不符合，需用 1.0.0 以内的版本

# 是否静默安装（值 1 表示位静默安装，其它如 0 表示非静默安装）
SILENT=1

# 版本设置
GCC_VERSION="13.1.0"
GMP_VERSION="6.2.0"
MPFR_VERSION="4.2.0"
MPC_VERSION="1.3.0"
M4_VERSION="1.4.9"
CMAKE_VERSION="3.27.0"

set +e # 遇到错误中止执行

# 安装 gcc 的依赖库之 gmp 库
# GMP 为“GNU MP Bignum Library”的缩写，是一个 GNU 开源数学运算库，国内镜像下载地址：
# 1) https://mirrors.tuna.tsinghua.edu.cn/gnu/gmp/
# 2) http://mirrors.nju.edu.cn/gnu/gmp/
# 3) http://mirrors.ustc.edu.cn/gnu/gmp/
function install_gmp()
{
    echo -e "\033[1;33mStart installing gmp ...\033[m"

    if test $SILENT -ne 1; then
        if test -d /usr/local/gmp; then
            echo -en "/usr/local/gmp already exists, do you want to overwrite? [\033[1;33myes\033[m/\033[1;33mno\033[m]"
            read -r -p " " input
            
            if test "$input" != "yes"; then
                return
            fi
        fi
    fi
    if test ! -d /usr/local/gmp-${GMP_VERSION}; then
        wget --no-check-certificate "https://mirrors.tuna.tsinghua.edu.cn/gnu/gmp/gmp-${GMP_VERSION}.tar.bz2"

        tar xjf gmp-${GMP_VERSION}.tar.bz2
        cd gmp-${GMP_VERSION}
        ./configure --prefix=/usr/local/gmp-${GMP_VERSION}
        make&&make install
        rm -f /usr/local/gmp&&ln -s /usr/local/gmp-${GMP_VERSION} /usr/local/gmp
        cd -
        
        export LD_LIBRARY_PATH=/usr/local/gmp/lib:$LD_LIBRARY_PATH
    fi
}

# 安装 gcc 的依赖库之 mpfr 库
# mpfr 是一个 GNU 开源大数运算库，它依赖 gmp，国内镜像下载地址：
# 1) https://mirrors.tuna.tsinghua.edu.cn/gnu/mpfr/
# 2) http://mirrors.nju.edu.cn/gnu/mpfr/
# 3) http://mirrors.ustc.edu.cn/gnu/mpfr/
function install_mpfr()
{
    echo -e "\033[1;33mStart installing mpfr ...\033[m"

    if test $SILENT -ne 1; then
        if test -d /usr/local/mpfr; then
            echo -en "/usr/local/mpfr already exists, do you want to overwrite? [\033[1;33myes\033[m/\033[1;33mno\033[m]"
            read -r -p " " input
            
            if test "$input" != "yes"; then
                return
            fi
        fi
    fi
    if test ! -d /usr/local/mpfr-${MPFR_VERSION}; then
        wget --no-check-certificate "https://mirrors.tuna.tsinghua.edu.cn/gnu/mpfr/mpfr-${MPFR_VERSION}.tar.bz2"

        tar xjf mpfr-${MPFR_VERSION}.tar.bz2
        cd mpfr-${MPFR_VERSION}
        ./configure --prefix=/usr/local/mpfr-${MPFR_VERSION} --with-gmp=/usr/local/gmp
        make&&make install
        rm -f /usr/local/mpfr&&ln -s /usr/local/mpfr-${MPFR_VERSION} /usr/local/mpfr
        cd -
        
        export LD_LIBRARY_PATH=/usr/local/mpfr/lib:$LD_LIBRARY_PATH
    fi
}

# 安装 gcc 的依赖库之 mpc 库
# mpc 是 GNU 的开源复杂数字算法，它依赖 gmp 和 mpfr，国内镜像下载地址：
# 1) https://mirrors.tuna.tsinghua.edu.cn/gnu/mpc/
# 2) http://mirrors.nju.edu.cn/gnu/mpc/
# 3) http://mirrors.ustc.edu.cn/gnu/mpc/
function install_mpc()
{
    echo -e "\033[1;33mStart installing mpc ...\033[m"

    if test $SILENT -ne 1; then
        if test -d /usr/local/mpc; then
            echo -en "/usr/local/mpc already exists, do you want to overwrite? [\033[1;33myes\033[m/\033[1;33mno\033[m]"
            read -r -p " " input
            
            if test "$input" != "yes"; then
                return
            fi
        fi
    fi
    if test ! -d /usr/local/mpc-${MPC_VERSION}; then
        wget --no-check-certificate "https://mirrors.tuna.tsinghua.edu.cn/gnu/mpc/mpc-${MPC_VERSION}.tar.gz"

        tar xzf mpc-${MPC_VERSION}.tar.gz
        cd mpc-${MPC_VERSION}
        ./configure --prefix=/usr/local/mpc-${MPC_VERSION} --with-gmp=/usr/local/gmp --with-mpfr=/usr/local/mpfr
        make&&make install
        rm -f /usr/local/mpc&&ln -s /usr/local/mpc-${MPC_VERSION} /usr/local/mpc
        cd -
        
        export LD_LIBRARY_PATH=/usr/local/mpc/lib:$LD_LIBRARY_PATH

        # 解决执行 gcc 的 configure 时错误”Building GCC requires GMP 4.2+, MPFR 3.1.0+ and MPC 0.8.0+.“
        sed -i '/#include <stdint.h>/a\#include <stdio.h>' /usr/local/mpc/include/mpc.h
    fi
}

# 升级编译工具 m4
# 国内镜像下载地址：
# 1) https://mirrors.tuna.tsinghua.edu.cn/gnu/m4/
# 2) http://mirrors.nju.edu.cn/gnu/m4/
# 3) http://mirrors.ustc.edu.cn/gnu/m4/
function update_m4()
{
    echo -e "\033[1;33mStart installing m4 ...\033[m"

    if test $SILENT -ne 1; then
        if test -d /usr/local/m4; then
            echo -en "/usr/local/m4 already exists, do you want to overwrite? [\033[1;33myes\033[m/\033[1;33mno\033[m]"
            read -r -p " " input
            
            if test "$input" != "yes"; then
                return
            fi
        fi
    fi
    if test ! -d /usr/local/m4-${M4_VERSION}; then    
        wget --no-check-certificate "https://mirrors.tuna.tsinghua.edu.cn/gnu/m4/m4-${M4_VERSION}.tar.bz2"

        tar xjf m4-${M4_VERSION}.tar.bz2
        cd m4-${M4_VERSION}
        ./configure --prefix=/usr/local/m4-${M4_VERSION}
        make&&make install
        rm -f /usr/local/m4&&ln -s /usr/local/m4-${M4_VERSION} /usr/local/m4
        cd -

        export PATH=/usr/local/m4/bin:$PATH # 使 m4 生效
    fi
}

# 安装 gcc
# 国内镜像下载地址：
# 1) https://mirrors.tuna.tsinghua.edu.cn/gnu/gcc/gcc-8.3.0/
# 2) http://mirrors.nju.edu.cn/gnu/gcc/gcc-8.3.0/
# 3) http://mirrors.ustc.edu.cn/gnu/gcc/gcc-8.3.0/
function install_gcc()
{
    echo -e "\033[1;33mStart installing gcc ...\033[m"

    if test $SILENT -ne 1; then
        if test -d /usr/local/gcc; then
            echo -en "/usr/local/gcc already exists, do you want to overwrite? [\033[1;33myes\033[m/\033[1;33mno\033[m]"
            read -r -p " " input
            
            if test "$input" != "yes"; then
                return
            fi
        fi
    fi
    if test ! -d /usr/local/gcc-${GCC_VERSION}; then
        # 13.1.0 新增支持 std::format
        wget --no-check-certificate "https://mirrors.tuna.tsinghua.edu.cn/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz"

        tar xzf gcc-${GCC_VERSION}.tar.gz
        cd gcc-${GCC_VERSION}
        # ./configure --prefix=/usr/local/gcc-13.1.0 --disable-multilib --with-mpfr=/usr/local/mpfr --with-gmp=/usr/local/gmp --with-mpc=/usr/local/mpc
        ./configure --prefix=/usr/local/gcc-${GCC_VERSION} --disable-multilib --with-mpfr=/usr/local/mpfr --with-gmp=/usr/local/gmp --with-mpc=/usr/local/mpc
        time make&&make install    
        rm -f /usr/local/gcc&&ln -s /usr/local/gcc-${GCC_VERSION} /usr/local/gcc
        cd -

        export PATH=/usr/local/gcc/bin:$PATH
        export LD_LIBRARY_PATH=/usr/local/gcc/lib64:$LD_LIBRARY_PATH
        export MANPATH=/usr/local/gcc/share/man:$MANPATH
    fi
}

# 验证 gcc
function verify_gcc()
{
    gcc --version
}

# 安装 gcc 的依赖库之 mpfr 库
install_gmp

# 安装 gcc 的依赖库之 mpfr 库
install_mpfr

# 安装 gcc 的依赖库之 mpc 库
install_mpc

# 升级编译工具 m4
update_m4

# 安装 gcc
install_gcc

# 验证 gcc
verify_gcc

# 恢复错误处理
set -e 
