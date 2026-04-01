# Writed by yijian (eyjian@qq.com, eyjian@gmail.com)

include_directories(../../include)
link_directories(../../src)
link_libraries(libmooon.a)
link_libraries(dl pthread rt z)

add_executable(ut_string_utils ut_string_utils.cpp)
add_executable(ut_tokener ut_tokener.cpp)
add_executable(test_args_parser test_args_parser.cpp)
