add_library(datasrv STATIC
        datasrv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../daqd/daqc_access.c
        )
target_include_directories(datasrv PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../../daqd
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        )
add_library(th::datasrv ALIAS datasrv)
