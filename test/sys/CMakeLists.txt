# Writed by yijian (eyjian@qq.com, eyjian@gmail.com)

include_directories(../../include)
link_directories(../../src)
link_libraries(libmooon.a)
link_libraries(dl pthread rt z)

add_executable(test_safe_logger test_safe_logger.cpp)
add_executable(ut_datetime_utils ut_datetime_utils.cpp)
add_executable(ut_event_queue ut_event_queue.cpp)
add_executable(ut_fs_utils ut_fs_utils.cpp)

if (MOOON_HAVE_LIBIDN)
    add_executable(curl_get test_curl_wrapper.cpp)
    target_link_libraries(curl_get libcurl.a libcares.a libidn.a libssl.a libcrypto.a)
endif ()
