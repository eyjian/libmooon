# Writed by yijian (eyjian@qq.com, eyjian@gmail.com)

# CMAKE_INSTALL_PREFIX
install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../include/mooon/helper        
        DESTINATION include/mooon
        PATTERN "*.h"
       )
