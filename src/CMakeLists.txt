# Writed by yijian (eyjian@qq.com, eyjian@gmail.com)

add_subdirectory(utils)
add_subdirectory(sys)
add_subdirectory(net)
add_subdirectory(helper)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include/mooon)

# libmooon.a
add_library(
    mooon
    ${MOOON_UTILS_SRC}
    ${MOOON_SYS_SRC}
    ${MOOON_NET_SRC}
)

# CMAKE_INSTALL_PREFIX
install(
        TARGETS mooon
        DESTINATION lib
       )
