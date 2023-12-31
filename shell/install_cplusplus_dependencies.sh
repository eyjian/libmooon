#!/bin/sh
# Writed by yijian on 2023/12/29
# 安装常用 C++ 开源库工具

# 安装目录
INSTALL_DIR=`echo "${INSTALL_DIR:-/usr/local/thirdparties}"`
# 是否静默安装（1 静默，0 交互）
SILENT_INSTALL=`echo "${SILENT_INSTALL:-0}"`

# 版本设置
CPLUSPLUS_VERSION=`echo "${CPLUSPLUS_VERSION:-23}"` # C++ 版本
CMAKE_VERSION=`echo "${CMAKE_VERSION:-3.28.1}"` # CMake 版本
BOOST_VERSION=1_84_0
OPENSSL_VERSION=3.2.0
LIBEVENT_VERSION=2.1.12
THRIFT_VERSION=0.19.0
CARES_VERSION=1.24.0
LIBSSH2_VERSION=1.11.0
CURL_VERSION=8.5.0
CGICC_VERSION=3.2.20
LIBRDKAFKA_VERSION=2.3.0
HIREDIS_VERSION=1.2.0
RAPIDJSON_VERSION=1.1.0
MYSQL_VERSION=5.6.17
SQLITE_VERSION=3.44.2
PROTOBUF_VERSION=25.1
GRPC_VERSION=1.60.0

# THRIFT 设置
THRIFT_CPLUSPLUS=yes
THRIFT_GO=no
THRIFT_JAVA=no
THRIFT_PHP=no
THRIFT_PYTHON=no
THRIFT_LUA=no
THRIFT_NODEJS=no

# 工作目录
workdir="`pwd`"
rm -f $workdir/install.log

install_cmake()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mcmake-$CMAKE_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/cmake-$CMAKE_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] cmake-$CMAKE_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/cmake-$CMAKE_VERSION/installed; then
        echo -e "$INSTALL_DIR/cmake-$CMAKE_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] cmake-$CMAKE_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/cmake-$CMAKE_VERSION \033[1;33mstarting\033[m"

        if test ! -f cmake-$CMAKE_VERSION.tar.gz; then
            wget --no-check-certificate "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.tar.gz"
        fi
        rm -fr cmake-$CMAKE_VERSION
        tar xzf cmake-$CMAKE_VERSION.tar.gz

        cd cmake-$CMAKE_VERSION
        ./configure --prefix=$INSTALL_DIR/cmake-$CMAKE_VERSION
        make&&make install

        if test -h $INSTALL_DIR/cmake; then
            rm -f $INSTALL_DIR/cmake
        fi
        ln -s $INSTALL_DIR/cmake-$CMAKE_VERSION $INSTALL_DIR/cmake
        touch $INSTALL_DIR/cmake-$CMAKE_VERSION/installed
        echo -e "$INSTALL_DIR/cmake-$CMAKE_VERSION \033[1;33msuccess\033[m"

        export PATH=$INSTALL_DIR/cmake/bin:$PATH
        export MANPATH=$INSTALL_DIR/cmake/share/cmake-$CMAKE_VERSION:$MANPATH
        echo -e "$INSTALL_DIR/cmake-$CMAKE_VERSION \033[1;33menabled\033[m"
        echo "[SUCCESS] cmake-$CMAKE_VERSION" >> $workdir/install.log

        return
        # 在使用 cmake 前需设置好下列所环境变量，否则 cmake 仍将使用默认目录下的 gcc 和 g++
        GCC_DIR=/usr/local
        export CC=$GCC_DIR/gcc/bin/gcc
        export CXX=$GCC_DIR/gcc/bin/g++
        export PATH=$GCC_DIR/gcc/bin:$PATH
        export LD_LIBRARY_PATH=$GCC_DIR/gcc/lib64:$LD_LIBRARY_PATH
        export LD_LIBRARY_PATH=$GCC_DIR/gmp/lib:$GCC_DIR/mpfr/lib:$GCC_DIR/mpc/lib:$LD_LIBRARY_PATH
    fi
}

install_boost()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mboost_$BOOST_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/boost_$BOOST_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] boost_$BOOST_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/boost_$BOOST_VERSION/installed; then
        echo -e "$INSTALL_DIR/boost_$BOOST_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] boost_$BOOST_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/boost_$BOOST_VERSION \033[1;33mstarting\033[m"

        if test ! -f boost_$BOOST_VERSION.tar.gz; then
            wget --no-check-certificate "https://boostorg.jfrog.io/artifactory/main/release/1.84.0/source/boost_$BOOST_VERSION.tar.gz"
        fi
        rm -fr boost_$BOOST_VERSION
        tar xzf boost_$BOOST_VERSION.tar.gz

        cd boost_$BOOST_VERSION
        ./bootstrap.sh --without-icu --without-libraries=python,graph,graph_parallel,mpi,wave
        ./b2 install threading=multi --prefix=$INSTALL_DIR/boost_$BOOST_VERSION --without-python --without-graph --without-graph_parallel --without-mpi --without-wave

        if test -h $INSTALL_DIR/boost; then
            rm -f $INSTALL_DIR/boost
        fi
        ln -s $INSTALL_DIR/boost_$BOOST_VERSION $INSTALL_DIR/boost
        touch $INSTALL_DIR/boost_$BOOST_VERSION/installed
        echo -e "$INSTALL_DIR/boost_$BOOST_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] boost_$BOOST_VERSION" >> $workdir/install.log
    fi
}

install_openssl()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mopenssl-$OPENSSL_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/openssl-$OPENSSL_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] openssl-$OPENSSL_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/openssl-$OPENSSL_VERSION/installed; then
        echo -e "$INSTALL_DIR/openssl-$OPENSSL_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] openssl-$OPENSSL_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/openssl-$OPENSSL_VERSION \033[1;33mstarting\033[m"

        if test ! -f openssl-$OPENSSL_VERSION.tar.gz; then
            wget --no-check-certificate "https://www.openssl.org/source/openssl-$OPENSSL_VERSION.tar.gz"
        fi
        rm -fr openssl-$OPENSSL_VERSION
        tar xzf openssl-$OPENSSL_VERSION.tar.gz

        cd openssl-$OPENSSL_VERSION
        ./config --prefix=$INSTALL_DIR/openssl-$OPENSSL_VERSION shared threads
        make&&make install

        if test -h $INSTALL_DIR/openssl; then
            rm -f $INSTALL_DIR/openssl
        fi
        ln -s $INSTALL_DIR/openssl-$OPENSSL_VERSION $INSTALL_DIR/openssl
        touch $INSTALL_DIR/openssl-$OPENSSL_VERSION/installed
        echo -e "$INSTALL_DIR/openssl-$OPENSSL_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] openssl-$OPENSSL_VERSION" >> $workdir/install.log
    fi

}

install_libevent()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mlibevent-$LIBEVENT_VERSION-stable\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable \033[1;33mskipped\033[m"
            echo "[SKIP] libevent-$LIBEVENT_VERSION-stable" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable/installed; then
        echo -e "$INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable \033[1;33minstalled\033[m"
        echo "[INSTALLED] libevent-$LIBEVENT_VERSION-stable" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable \033[1;33mstarting\033[m"

        if test ! -f libevent-$LIBEVENT_VERSION-stable.tar.gz; then
            wget --no-check-certificate "https://github.com/libevent/libevent/releases/download/release-$LIBEVENT_VERSION-stable/libevent-$LIBEVENT_VERSION-stable.tar.gz"
        fi
        rm -fr libevent-$LIBEVENT_VERSION-stable
        tar xzf libevent-$LIBEVENT_VERSION-stable.tar.gz

        cd libevent-$LIBEVENT_VERSION-stable
        ./configure --prefix=$INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable
        make&&make install

        if test -h $INSTALL_DIR/libevent; then
            rm -f $INSTALL_DIR/libevent
        fi
        ln -s $INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable $INSTALL_DIR/libevent
        touch $INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable/installed
        echo -e "$INSTALL_DIR/libevent-$LIBEVENT_VERSION-stable \033[1;33msuccess\033[m"
        echo "[SUCCESS] libevent-$LIBEVENT_VERSION-stable" >> $workdir/install.log
    fi
}

install_thrift()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mthrift-$THRIFT_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/thrift-$THRIFT_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] thrift-$THRIFT_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/thrift-$THRIFT_VERSION/installed; then
        echo -e "$INSTALL_DIR/thrift-$THRIFT_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] thrift-$THRIFT_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/thrift-$THRIFT_VERSION \033[1;33mstarting\033[m"

        if test ! -f thrift-$THRIFT_VERSION.tar.gz; then
            wget --no-check-certificate "https://archive.apache.org/dist/thrift/$THRIFT_VERSION/thrift-$THRIFT_VERSION.tar.gz" -O thrift-$THRIFT_VERSION.tar.gz
        fi
        rm -fr thrift-$THRIFT_VERSION
        tar xzf thrift-$THRIFT_VERSION.tar.gz

        cd thrift-$THRIFT_VERSION
        ./configure --prefix=$INSTALL_DIR/thrift-$THRIFT_VERSION --with-boost=$INSTALL_DIR/boost --with-libevent=$INSTALL_DIR/libevent --with-openssl=$INSTALL_DIR/openssl --with-cpp=$THRIFT_CPLUSPLUS --with-go=$THRIFT_GO --with-java=THRIFT_JAVA --with-python=THRIFT_PYTHON --with-php=THRIFT_PHP --with-lua=$THRIFT_LUA --with-nodejs=THRIFT_NODEJS --with-qt5=no --with-c_glib=no --with-kotlin=no --with-erlang=no --with-nodets=no --with-py3=no --with-perl=no --with-php_extension=no --with-dart=no --with-ruby=no --with-swift=no --with-rs=no --with-cl=no --with-haxe=no --with-netstd=no --with-d=no --enable-tests=no --enable-tutorial=no --enable-coverage=no
        # 文件 config.status 控制使用哪个 c++ 版本
        sed -i '/S\["CXX"\]=/c\S["CXX"]="g++ -std=c++'$CPLUSPLUS_VERSION'"' config.status
        sed -i '/S[\"CXXCPP"\]=/c\S["CXXCPP"]="g++ -E -std=c++'$CPLUSPLUS_VERSION'"' config.status
        make&&make install

        if test -h $INSTALL_DIR/thrift; then
            rm -f $INSTALL_DIR/thrift
        fi
        ln -s $INSTALL_DIR/thrift-$THRIFT_VERSION $INSTALL_DIR/thrift
        touch $INSTALL_DIR/thrift-$THRIFT_VERSION/installed
        echo -e "$INSTALL_DIR/thrift-$THRIFT_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] thrift-$THRIFT_VERSION" >> $workdir/install.log
    fi
}

install_cares()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mc-ares-$CARES_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/c-ares-$CARES_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] c-ares-$CARES_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/c-ares-$CARES_VERSION/installed; then
        echo -e "$INSTALL_DIR/c-ares-$CARES_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] c-ares-$CARES_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/c-ares-$CARES_VERSION \033[1;33mstarting\033[m"

        if test ! -f c-ares-$CARES_VERSION.tar.gz; then
            wget --no-check-certificate "https://c-ares.org/download/c-ares-$CARES_VERSION.tar.gz"
        fi
        rm -fr c-ares-$CARES_VERSION
        tar xzf c-ares-$CARES_VERSION.tar.gz

        cd c-ares-$CARES_VERSION
        ./configure --prefix=$INSTALL_DIR/c-ares-$CARES_VERSION --enable-tests=false
        make&&make install

        if test $INSTALL_DIR/c-ares; then
            rm -f $INSTALL_DIR/c-ares
        fi
        ln -s $INSTALL_DIR/c-ares-$CARES_VERSION $INSTALL_DIR/c-ares
        touch $INSTALL_DIR/c-ares-$CARES_VERSION/installed
        echo -e "$INSTALL_DIR/c-ares-$CARES_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] c-ares-$CARES_VERSION" >> $workdir/install.log
    fi
}

install_libssh2()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mlibssh2-$LIBSSH2_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/libssh2-$LIBSSH2_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] libssh2-$LIBSSH2_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/libssh2-$LIBSSH2_VERSION/installed; then
        echo -e "$INSTALL_DIR/libssh2-$LIBSSH2_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] libssh2-$LIBSSH2_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/libssh2-$LIBSSH2_VERSION \033[1;33mstarging\033[m"

        if test ! -f libssh2-$LIBSSH2_VERSION.tar.gz; then
            cmd="wget --no-check-certificate \"https://libssh2.org/download/libssh2-$LIBSSH2_VERSION.tar.gz\""
            echo "$cmd"
            sh -c "$cmd"
        fi
        tar xzf libssh2-$LIBSSH2_VERSION.tar.gz

        cd libssh2-$LIBSSH2_VERSION
        ./configure --prefix=$INSTALL_DIR/libssh2-$LIBSSH2_VERSION --with-libssl-prefix=$INSTALL_DIR/openssl
        make&&make install

        if test -h $INSTALL_DIR/libssh2; then
            rm -f $INSTALL_DIR/libssh2
        fi
        ln -s $INSTALL_DIR/libssh2-$LIBSSH2_VERSION $INSTALL_DIR/libssh2
        touch $INSTALL_DIR/libssh2-$LIBSSH2_VERSION/installed
        echo -e "$INSTALL_DIR/libssh2-$LIBSSH2_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] libssh2-$LIBSSH2_VERSION" >> $workdir/install.log
    fi
}

install_curl()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mcurl-$CURL_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/curl-$CURL_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] curl-$CURL_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/curl-$CURL_VERSION/installed; then
        echo -e "$INSTALL_DIR/curl-$CURL_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] curl-$CURL_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/curl-$CURL_VERSION \033[1;33mstarting\033[m"

        if test ! -f curl-$CURL_VERSION.tar.gz; then
            wget --no-check-certificate "https://curl.se/download/curl-$CURL_VERSION.tar.gz"
        fi
        rm -fr curl-$CURL_VERSION
        tar xzf curl-$CURL_VERSION.tar.gz

        cd curl-$CURL_VERSION
        ./configure --prefix=$INSTALL_DIR/curl-$CURL_VERSION --enable-ares=$INSTALL_DIR/c-ares --with-ssl=$INSTALL_DIR/openssl # --with-libssh2=$INSTALL_DIR/libssh2
        make&&make install

        if test -h $INSTALL_DIR/curl; then
            rm -f $INSTALL_DIR/curl
        fi
        ln -s $INSTALL_DIR/curl-$CURL_VERSION $INSTALL_DIR/curl
        touch $INSTALL_DIR/curl-$CURL_VERSION/installed
        echo -e "$INSTALL_DIR/curl-$CURL_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] curl-$CURL_VERSION" >> $workdir/install.log
    fi
}

install_cgicc()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mcgicc-$CGICC_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/cgicc-$CGICC_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] cgicc-$CGICC_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/cgicc-$CGICC_VERSION/installed; then
        echo -e "$INSTALL_DIR/cgicc-$CGICC_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] cgicc-$CGICC_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/cgicc-$CGICC_VERSION \033[1;33mstarting\033[m"

        if test ! -f cgicc-$CGICC_VERSION.tar.gz; then
            wget "http://ftp.gnu.org/gnu/cgicc/cgicc-3.2.20.tar.gz"
        fi
        rm -fr cgicc-$CGICC_VERSION
        tar xzf cgicc-$CGICC_VERSION.tar.gz

        cd cgicc-$CGICC_VERSION
        ./configure --prefix=$INSTALL_DIR/cgicc-$CGICC_VERSION
        make&&make install

        if test -h $INSTALL_DIR/cgicc; then
            rm -f $INSTALL_DIR/cgicc
        fi
        ln -s $INSTALL_DIR/cgicc-$CGICC_VERSION $INSTALL_DIR/cgicc
        touch $INSTALL_DIR/cgicc-$CGICC_VERSION/installed
        echo -e "$INSTALL_DIR/cgicc-$CGICC_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] cgicc-$CGICC_VERSION" >> $workdir/install.log
    fi
}

install_librdkafka()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mlibrdkafka-$LIBRDKAFKA_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] librdkafka-$LIBRDKAFKA_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION/installed; then
        echo -e "$INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] librdkafka-$LIBRDKAFKA_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION \033[1;33mstarting\033[m"

        if test ! -f librdkafka-$LIBRDKAFKA_VERSION.tar.gz; then
            wget --no-check-certificate "https://github.com/confluentinc/librdkafka/archive/refs/tags/v2.3.0.tar.gz" -O librdkafka-$LIBRDKAFKA_VERSION.tar.gz
        fi
        rm -fr librdkafka-$LIBRDKAFKA_VERSION
        tar xzf librdkafka-$LIBRDKAFKA_VERSION.tar.gz

        cd librdkafka-$LIBRDKAFKA_VERSION
        ./configure --prefix=$INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION
        make&&make install

        if test -h $INSTALL_DIR/librdkafka; then
            rm -f $INSTALL_DIR/librdkafka
        fi
        ln -s $INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION $INSTALL_DIR/librdkafka
        touch $INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION/installed
        echo -e "$INSTALL_DIR/librdkafka-$LIBRDKAFKA_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] librdkafka-$LIBRDKAFKA_VERSION" >> $workdir/install.log
    fi
}

install_hiredis()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mhiredis-$HIREDIS_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/hiredis-$HIREDIS_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] hiredis-$HIREDIS_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/hiredis-$HIREDIS_VERSION/installed; then
        echo -e "$INSTALL_DIR/hiredis-$HIREDIS_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] hiredis-$HIREDIS_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/hiredis-$HIREDIS_VERSION \033[1;33mstarting\033[m"

        if test ! -f hiredis-$HIREDIS_VERSION.tar.gz; then
            wget --no-check-certificate "https://github.com/redis/hiredis/archive/refs/tags/v$HIREDIS_VERSION.tar.gz" -O hiredis-$HIREDIS_VERSION.tar.gz
        fi
        rm -fr hiredis-$HIREDIS_VERSION
        tar xzf hiredis-$HIREDIS_VERSION.tar.gz

        cd hiredis-$HIREDIS_VERSION
        make&&make install PREFIX=$INSTALL_DIR/hiredis-$HIREDIS_VERSION

        if test -h $INSTALL_DIR/hiredis; then
            rm -f $INSTALL_DIR/hiredis
        fi
        ln -s $INSTALL_DIR/hiredis-$HIREDIS_VERSION $INSTALL_DIR/hiredis
        touch $INSTALL_DIR/hiredis-$HIREDIS_VERSION/installed
        echo -e "$INSTALL_DIR/hiredis-$HIREDIS_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] hiredis-$HIREDIS_VERSION" >> $workdir/install.log
    fi
}

install_rapidjson()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mrapidjson-$RAPIDJSON_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] rapidjson-$RAPIDJSON_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION/installed; then
        echo -e "$INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] rapidjson-$RAPIDJSON_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION \033[1;33mstarting\033[m"

        if test ! -f rapidjson-$RAPIDJSON_VERSION.tar.gz; then
            wget --no-check-certificate "https://github.com/Tencent/rapidjson/archive/refs/tags/v$RAPIDJSON_VERSION.tar.gz" -O rapidjson-$RAPIDJSON_VERSION.tar.gz
        fi
        rm -fr rapidjson-$RAPIDJSON_VERSION
        tar xzf rapidjson-$RAPIDJSON_VERSION.tar.gz

        cd rapidjson-$RAPIDJSON_VERSION
        cmake -DCMAKE_VERBOSE_MAKEFILE=on -DCMAKE_CXX_STANDARD=$CPLUSPLUS_VERSION -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION \
-DRAPIDJSON_BUILD_EXAMPLES=ON -DRAPIDJSON_BUILD_TESTS=OFF -DRAPIDJSON_BUILD_DOC=OFF -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF \
-DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations -Wno-class-memaccess -Wno-implicit-fallthrough" .
        make&&make install

        if test -h $INSTALL_DIR/rapidjson; then
            rm -f $INSTALL_DIR/rapidjson
        fi
        ln -s $INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION $INSTALL_DIR/rapidjson
        touch $INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION/installed
        echo -e "$INSTALL_DIR/rapidjson-$RAPIDJSON_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] rapidjson-$RAPIDJSON_VERSION" >> $workdir/install.log
    fi
}

install_msyql()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mmysql-$MYSQL_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/mysql-$MYSQL_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] mysql-$MYSQL_VERSION" >> $workdir/install.log
            return
        fi
    fi

    if test -f $INSTALL_DIR/mysql-$MYSQL_VERSION/installed; then
        echo -e "$INSTALL_DIR/mysql-$MYSQL_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] mysql-$MYSQL_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/mysql-$MYSQL_VERSION \033[1;33mstarting\033[m"

        if test ! -f mysql-$MYSQL_VERSION-linux-glibc2.5-x86_64.tar.gz; then
            wget --no-check-certificate "https://github.com/eyjian/bin-package/raw/main/mysql-$MYSQL_VERSION-linux-glibc2.5-x86_64.tar.gz"
        fi
        rm -fr mysql-$MYSQL_VERSION-linux-glibc2.5-x86_64
        tar xzf mysql-$MYSQL_VERSION-linux-glibc2.5-x86_64.tar.gz

        cp -R mysql-$MYSQL_VERSION-linux-glibc2.5-x86_64 $INSTALL_DIR/mysql-$MYSQL_VERSION
        if test -h $INSTALL_DIR/mysql; then
            rm -f $INSTALL_DIR/mysql
        fi
        ln -s $INSTALL_DIR/mysql-$MYSQL_VERSION $INSTALL_DIR/mysql
        touch $INSTALL_DIR/mysql-$MYSQL_VERSION/installed
        echo -e "$INSTALL_DIR/mysql-$MYSQL_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] mysql-$MYSQL_VERSION" >> $workdir/install.log
    fi
}

install_sqlite()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33msqlite3-$SQLITE_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/sqlite3-$SQLITE_VERSION \033[1;33mskipped\033[m"
            echo "[SKIP] sqlite3-$SQLITE_VERSION" >> $workdir/install.log
            return
        fi
    fi


    if test -f $INSTALL_DIR/sqlite3-$SQLITE_VERSION/installed; then
        echo -e "$INSTALL_DIR/sqlite3-$SQLITE_VERSION \033[1;33minstalled\033[m"
        echo "[INSTALLED] sqlite3-$SQLITE_VERSION" >> $workdir/install.log
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/sqlite3-$SQLITE_VERSION \033[1;33mstarting\033[m"

        if test ! -f sqlite3-$SQLITE_VERSION.tar.gz; then
            wget --no-check-certificate "https://www.sqlite.org/2023/sqlite-autoconf-3440200.tar.gz" -O sqlite3-$SQLITE_VERSION.tar.gz
        fi
        rm -fr sqlite-autoconf-*
        tar xzf sqlite3-$SQLITE_VERSION.tar.gz
        mv sqlite-autoconf-* sqlite3-$SQLITE_VERSION

        cd sqlite3-$SQLITE_VERSION
        ./configure --prefix=$INSTALL_DIR/sqlite3-$SQLITE_VERSION
        make&&make install
        libtool --finish $INSTALL_DIR/sqlite3-$SQLITE_VERSION/lib

        ln -s $INSTALL_DIR/sqlite3-$SQLITE_VERSION $INSTALL_DIR/sqlite3
        touch $INSTALL_DIR/sqlite3-$SQLITE_VERSION/installed
        echo -e "$INSTALL_DIR/sqlite3-$SQLITE_VERSION \033[1;33msuccess\033[m"
        echo "[SUCCESS] sqlite3-$SQLITE_VERSION" >> $workdir/install.log
    fi
}

install_protobuf()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mprotobuf-$PROTOBUF_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/protobuf-$PROTOBUF_VERSION \033[1;33mskipped\033[m"
            return
        fi
    fi

    if test -f $INSTALL_DIR/protobuf-$PROTOBUF_VERSION/installed; then
        echo -e "$INSTALL_DIR/protobuf-$PROTOBUF_VERSION \033[1;33minstalled\033[m"
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/protobuf-$PROTOBUF_VERSION \033[1;33mstarting\033[m"

        if test ! -f protobuf-$PROTOBUF_VERSION.tar.gz; then
            wget --no-check-certificate "https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOBUF_VERSION/protobuf-$PROTOBUF_VERSION.tar.gz"
        fi
        rm -fr protobuf-$PROTOBUF_VERSION
        tar xzf protobuf-$PROTOBUF_VERSION.tar.gz

        cd protobuf-$PROTOBUF_VERSION
        cmake -DCMAKE_CXX_STANDARD=$CPLUSPLUS_VERSION -DCMAKE_BUILD_TYPE=Debug -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/protobuf-$PROTOBUF_VERSION .
        make&&make install

        if test -h $INSTALL_DIR/protobuf; then
            rm -f $INSTALL_DIR/protobuf
        fi
        ln -s $INSTALL_DIR/protobuf-$PROTOBUF_VERSION $INSTALL_DIR/protobuf
        touch $INSTALL_DIR/protobuf-$PROTOBUF_VERSION/installed
        echo -e "$INSTALL_DIR/protobuf-$PROTOBUF_VERSION \033[1;33msuccess\033[m"
    fi
}

install_grpc()
{
    if test $SILENT_INSTALL -eq 0; then
        echo -en "Install \033[1;33mgrpc-$GRPC_VERSION\033[m? ENTER or YES to install, NO or no to skip installing.\n"
        read -r -p "" yes_or_no
        if test "X$yes_or_no" = "Xno" -o "X$yes_or_no" = "XNO"; then
            echo -e "$INSTALL_DIR/grpc-$GRPC_VERSION \033[1;33mskipped\033[m"
            return
        fi
    fi

    if test -f $INSTALL_DIR/grpc-$GRPC_VERSION/installed; then
        echo -e "$INSTALL_DIR/grpc-$GRPC_VERSION \033[1;33minstalled\033[m"
    else
        cd "$workdir"
        echo -e "$INSTALL_DIR/grpc-$GRPC_VERSION \033[1;33mstarting\033[m"

        if test ! -d grpc; then
            git clone --recurse-submodules -b v$GRPC_VERSION --depth 1 --shallow-submodules https://github.com/grpc/grpc
        fi

        cd grpc
        mkdir -p cmake/build
        pushd cmake/build
        cmake -DCMAKE_CXX_STANDARD=$CPLUSPLUS_VERSION -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/grpc-$GRPC_VERSION ../..
        make -j 4
        #make install
        popd

        if test -h $INSTALL_DIR/grpc; then
            rm -f $INSTALL_DIR/grpc
        fi
        ln -s $INSTALL_DIR/grpc-$GRPC_VERSION $INSTALL_DIR/grpc
        touch $INSTALL_DIR/grpc-$GRPC_VERSION/installed
        echo -e "$INSTALL_DIR/grpc-$GRPC_VERSION \033[1;33msuccess\033[m"
    fi
}

set_gcc()
{
    echo "Set gcc"
    set -x

    # 当为 g++ 同时指定了多个 -std 选项时，实际生效的是最后一个
    #export CXX="g++ -std=c++$CPLUSPLUS_VERSION" # C++ compiler command
    export CXXCPP="g++ -E -std=c++$CPLUSPLUS_VERSION" # C++ preprocessor
    export CXXFLAGS="-std=c++$CPLUSPLUS_VERSION"    # C++ compiler flags
    #export AM_CXXFLAGS="-std=c++$CPLUSPLUS_VERSION"
    set +x
    echo ""
}

main()
{
    # 准备好安装目录
    if test ! -d $INSTALL_DIR; then
        mkdir -p $INSTALL_DIR
    fi

    set -e
    set_gcc

    install_cmake
    install_boost
    install_openssl
    install_libevent
    install_thrift
    install_cares
    install_libssh2
    install_curl
    install_cgicc
    install_librdkafka
    install_hiredis
    install_rapidjson
    install_msyql
    install_sqlite
    #install_protobuf
    #install_grpc

    set +e
}

main
