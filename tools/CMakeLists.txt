# Writed by yijian (eyjian@qq.com, eyjian@gmail.com)

include_directories(.)
include_directories(../include)
include_directories(../include/mooon)
link_directories(../src)
link_libraries(dl pthread rt z)

if (MOOON_HAVE_OPENSSL)
   	 # 计算md5工具
    add_executable(md5 md5.cpp)
    target_link_libraries(md5 libmooon.a libcrypto.a)
    
    add_executable(base64 base64.cpp)
    target_link_libraries(base64 libmooon.a libcrypto.a)
    
    add_executable(sha sha.cpp)
    target_link_libraries(sha libmooon.a libcrypto.a)
endif ()

# 硬盘性能测试工具
add_executable(disk_benchmark disk_benchmark.cpp)
target_link_libraries(disk_benchmark libmooon.a)

# pidof
add_executable(pidof pidof.cpp)
target_link_libraries(pidof libmooon.a)

# killall
add_executable(killall killall.cpp)
target_link_libraries(killall libmooon.a)

if (MOOON_HAVE_LIBSSH2)
	# 远程命令工具
	add_executable(mooon_ssh mooon_ssh.cpp)
	target_link_libraries(mooon_ssh libmooon.a libssh2.a libcrypto.a)
	
	# 批量上传工具
	add_executable(mooon_upload mooon_upload.cpp)
	target_link_libraries(mooon_upload libmooon.a libssh2.a libcrypto.a)

   	 # 下载工具
    add_executable(mooon_download mooon_download.cpp)
    target_link_libraries(mooon_download libmooon.a libssh2.a libcrypto.a)
    
	# CMAKE_INSTALL_PREFIX
	install(
	        TARGETS mooon_ssh mooon_upload mooon_download
	        DESTINATION bin
	)
endif ()

# r3c_stress
if (MOOON_HAVE_R3C)
    # r3c_stress
    add_executable(r3c_stress r3c_stress.cpp)
    target_link_libraries(r3c_stress libmooon.a libr3c.a libhiredis.a)
    
    # redis_queue_mover
    add_executable(redis_queue_mover redis_queue_mover.cpp)
    target_link_libraries(redis_queue_mover libmooon.a libr3c.a libhiredis.a)

    # CMAKE_INSTALL_PREFIX
    install(
            TARGETS r3c_stress redis_queue_mover
            DESTINATION bin
	)
endif ()

# hbase_stress
if (MOOON_HAVE_THRIFT)
    exec_program(${THRIFT_HOME}/bin/thrift ARGS --gen cpp -r -out ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/hbase.thrift)
    exec_program(rm ARGS ${CMAKE_CURRENT_SOURCE_DIR}/THBaseService_server.skeleton.cpp)

    add_executable(hbase_stress hbase_stress.cpp THBaseService.cpp hbase_constants.cpp hbase_types.cpp)
    target_link_libraries(hbase_stress libmooon.a libthrift.a)
    
    add_executable(hbase_scan hbase_scan.cpp THBaseService.cpp hbase_constants.cpp hbase_types.cpp)
    target_link_libraries(hbase_scan libmooon.a libthrift.a)
    
    # CMAKE_INSTALL_PREFIX
    install(
            TARGETS hbase_stress hbase_scan
            DESTINATION bin
	)      
endif ()

# mysql_escape_test
if (MOOON_HAVE_MYSQL)    
    add_executable(mysql_escape_test mysql_escape_test.cpp)
    target_link_libraries(mysql_escape_test libmooon.a libmysqlclient.a)
    install( # CMAKE_INSTALL_PREFIX
        TARGETS mysql_escape_test
        DESTINATION bin
    ) 
           
    add_executable(mysql_table_copy mysql_table_copy.cpp)
    target_link_libraries(mysql_table_copy libmooon.a libmysqlclient.a)
    install( # CMAKE_INSTALL_PREFIX
        TARGETS mysql_table_copy
        DESTINATION bin
    )
endif ()

if (MOOON_HAVE_ZOOKEEPER)
    # zkupload
    add_executable(zkupload zk_upload.cpp)
    target_link_libraries(zkupload libmooon.a libzookeeper_mt.a)
    
    # zkdownload
    add_executable(zkdownload zk_download.cpp)
    target_link_libraries(zkdownload libmooon.a libzookeeper_mt.a)
    
    # CMAKE_INSTALL_PREFIX
    install(
            TARGETS zkupload zkdownload
            DESTINATION bin
	) 
endif ()

if (MOOON_HAVE_CURL)
    # curl_download
    add_executable(curl_download curl_download.cpp)
    target_link_libraries(curl_download libmooon.a libcurl.a libcares.a libidn.a libssl.a libcrypto.a)
endif ()

# CMAKE_INSTALL_PREFIX
install(
        TARGETS disk_benchmark
        DESTINATION bin
)

if (MOOON_HAVE_LIBRDKAFKA)
	add_executable(kafka_consumer kafka_consumer.cpp)
    target_link_libraries(kafka_consumer libmooon.a librdkafka++.a librdkafka.a libssl.a libcrypto.a)
    
	# CMAKE_INSTALL_PREFIX
    install(
            TARGETS kafka_consumer
            DESTINATION bin
	) 
endif ()

if (MOOON_HAVE_R3C AND MOOON_HAVE_LIBRDKAFKA)
	# redis2kafka
	add_executable(redis2kafka redis2kafka.cpp)
	target_link_libraries(redis2kafka libmooon.a libr3c.a libhiredis.a librdkafka++.a librdkafka.a libssl.a libcrypto.a)
	
	# kafka2redis
	add_executable(kafka2redis kafka2redis.cpp)
	target_link_libraries(kafka2redis libmooon.a libr3c.a libhiredis.a librdkafka++.a librdkafka.a libssl.a libcrypto.a)
	
	# CMAKE_INSTALL_PREFIX
	install(
		TARGETS redis2kafka kafka2redis
		DESTINATION bin
	) 
endif ()
