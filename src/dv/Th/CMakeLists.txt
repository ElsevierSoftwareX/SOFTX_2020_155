add_library(datasrv STATIC
        datasrv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../daqd/daqc_access.c
        )
target_include_directories(datasrv PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../../daqd
        )
add_library(th::datasrv ALIAS datasrv)


add_library(datasrv_nds STATIC
        datasrv_nds.cc
        gpstime.h
        )
target_include_directories(datasrv_nds PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../../daqd
        )
target_link_libraries(datasrv_nds PUBLIC nds2client::cxx)
add_library(th::datasrv_nds ALIAS datasrv_nds)

add_executable(gpstime_test testing/gpstime_test.cc)
target_link_libraries(gpstime_test PUBLIC th::datasrv_nds ${CMAKE_THREAD_LIBS_INIT})
add_test(name test_gpstime command gpstime_test )
